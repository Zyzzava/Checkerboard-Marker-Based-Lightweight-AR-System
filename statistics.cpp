#include "statistics.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>

// HELPER: Calculate Mean and Standard Deviation
static std::pair<double, double> getMeanStdDev(const std::vector<double> &v)
{
    if (v.empty())
        return {0.0, 0.0};
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / v.size();
    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    double stddev = std::sqrt(std::max(0.0, sq_sum / v.size() - mean * mean));
    return {mean, stddev};
}

// 1. Compute Performance Summary
nlohmann::json SessionStats::computePerformance() const
{
    std::vector<double> times;
    times.reserve(frames.size());
    for (const auto &f : frames)
        times.push_back(f.frameTimeMs);

    auto [mean, stddev] = getMeanStdDev(times);
    double fps = (mean > 0) ? 1000.0 / mean : 0.0;

    return {
        {"mean_frame_time_ms", mean},
        {"stddev_frame_time_ms", stddev},
        {"fps", fps}};
}

// 2. Compute Robustness Summary
nlohmann::json SessionStats::computeDetectionRobustness() const
{
    int success = 0;
    int fail = 0;
    int maxFailStreak = 0;
    int currFailStreak = 0;
    int failureStreakCount = 0;

    for (const auto &f : frames)
    {
        if (f.poseSuccess)
        {
            if (currFailStreak > 0)
            {
                failureStreakCount++; // A streak just ended
            }
            if (currFailStreak > maxFailStreak)
            {
                maxFailStreak = currFailStreak;
            }
            currFailStreak = 0;
            success++;
        }
        else
        {
            currFailStreak++;
            fail++;
        }
    }
    // Check if the session ended on a streak
    if (currFailStreak > maxFailStreak)
        maxFailStreak = currFailStreak;

    double rate = frames.empty() ? 0.0 : (double)success / frames.size();

    return {
        {"success_rate", rate},
        {"total_failures", fail},
        {"max_failure_streak", maxFailStreak},
        {"failure_streak_count", failureStreakCount}};
}

// 3. Compute Pose Stability Summary
nlohmann::json SessionStats::computePoseStability() const
{
    // A. Collect valid poses
    std::vector<cv::Mat> valid_tvecs;
    std::vector<cv::Mat> valid_Rs; // Rotations in Matrix form

    for (const auto &f : frames)
    {
        if (f.poseSuccess)
        {
            valid_tvecs.push_back(f.tvec);
            cv::Mat R;
            cv::Rodrigues(f.rvec, R);
            valid_Rs.push_back(R);
        }
    }

    if (valid_tvecs.empty())
    {
        return {
            {"translation_mean_error", 0.0},
            {"translation_stddev", 0.0},
            {"rotation_mean_error_rad", 0.0},
            {"rotation_stddev_rad", 0.0}};
    }

    // B. Calculate Mean Translation
    cv::Mat t_sum = cv::Mat::zeros(3, 1, CV_64F);
    for (const auto &t : valid_tvecs)
        t_sum += t;
    cv::Mat t_mean = t_sum / (double)valid_tvecs.size();

    // C. Calculate Mean Rotation (SVD Method)
    cv::Mat R_sum = cv::Mat::zeros(3, 3, CV_64F);
    for (const auto &R : valid_Rs)
        R_sum += R;
    R_sum /= (double)valid_Rs.size();

    cv::Mat U, S, Vt;
    cv::SVD::compute(R_sum, S, U, Vt);
    cv::Mat R_mean = U * Vt;

    // D. Calculate Deviations (Jitter)
    std::vector<double> t_errors;
    std::vector<double> r_errors;

    for (size_t i = 0; i < valid_tvecs.size(); ++i)
    {
        // Translation Error (Euclidean distance from mean)
        t_errors.push_back(cv::norm(valid_tvecs[i] - t_mean));

        // Rotation Error (Geodesic angle from mean)
        cv::Mat R_diff = R_mean.t() * valid_Rs[i];
        double trace = cv::trace(R_diff)[0];
        // Clamp to avoid numerical errors going slightly outside [-1, 1]
        double val = (trace - 1.0) / 2.0;
        val = std::max(-1.0, std::min(1.0, val));
        r_errors.push_back(std::acos(val));
    }

    auto [meanT, stdT] = getMeanStdDev(t_errors);
    auto [meanR, stdR] = getMeanStdDev(r_errors);

    return {
        {"translation_mean_error", meanT},
        {"translation_stddev", stdT},
        {"rotation_mean_error_rad", meanR},
        {"rotation_stddev_rad", stdR}};
}

// 4. Export to JSON (Orchestrator)
nlohmann::json SessionStats::toJson() const
{
    nlohmann::json root;

    // 1. Add Summary Section (using the functions above)
    root["summary"] = {
        {"performance", computePerformance()},
        {"robustness", computeDetectionRobustness()},
        {"pose_stability", computePoseStability()}};

    // 2. Prepare for Per-Frame Calculations
    // We need to re-calculate the Mean Pose to generate per-frame delta values.
    // (Repeating logic here to avoid changing the header file with private members)
    cv::Mat mean_t = cv::Mat::zeros(3, 1, CV_64F);
    cv::Mat mean_R = cv::Mat::eye(3, 3, CV_64F);
    std::vector<cv::Mat> valid_Rs;
    int valid_count = 0;

    for (const auto &f : frames)
    {
        if (f.poseSuccess)
        {
            mean_t += f.tvec;
            cv::Mat R;
            cv::Rodrigues(f.rvec, R);
            valid_Rs.push_back(R);
            valid_count++;
        }
    }

    if (valid_count > 0)
    {
        mean_t /= (double)valid_count;
        cv::Mat R_sum = cv::Mat::zeros(3, 3, CV_64F);
        for (const auto &R : valid_Rs)
            R_sum += R;
        R_sum /= (double)valid_count;
        cv::Mat U, S, Vt;
        cv::SVD::compute(R_sum, S, U, Vt);
        mean_R = U * Vt;
    }

    // 3. Build Per-Frame Array
    root["frames"] = nlohmann::json::array();

    for (size_t i = 0; i < frames.size(); ++i)
    {
        const auto &f = frames[i];
        nlohmann::json entry;

        // Basic Info
        entry["frame_id"] = f.frame_id; // Using stored ID or index
        entry["timestamp"] = f.timestamp;
        entry["success"] = f.poseSuccess;
        entry["perf_time_ms"] = f.frameTimeMs;

        // Stability Stats (Jitter relative to mean)
        if (f.poseSuccess && valid_count > 0)
        {
            // Translation Jitter
            entry["stab_trans_jitter"] = cv::norm(f.tvec - mean_t);

            // Rotation Jitter
            cv::Mat R_curr;
            cv::Rodrigues(f.rvec, R_curr);
            cv::Mat R_diff = mean_R.t() * R_curr;
            double trace = cv::trace(R_diff)[0];
            double val = std::max(-1.0, std::min(1.0, (trace - 1.0) / 2.0));
            entry["stab_rot_jitter_rad"] = std::acos(val);
        }
        else
        {
            entry["stab_trans_jitter"] = nullptr;
            entry["stab_rot_jitter_rad"] = nullptr;
        }

        root["frames"].push_back(entry);
    }

    return root;
}