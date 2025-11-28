import json
import os
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

# --- Configuration ---
DPI = 300
OUTPUT_DIR = "data/figures"

# Set a consistent color scheme
COLORS = {'checkerboard': '#1f77b4', 'nft': '#ff7f0e'} # Blue vs Orange

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def load_json_df(filepath, test_type, method):
    if not os.path.exists(filepath):
        print(f"Warning: File {filepath} not found. Skipping.")
        return None
    
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None

    frames = data.get("frames", [])
    if not frames:
        return None

    df = pd.DataFrame(frames)
    df['test_type'] = test_type
    df['method'] = method
    df['label'] = f"{method.capitalize()} ({test_type.capitalize()})"
    return df

def plot_performance_all(df):
    # Get unique test types to determine grid layout
    test_types = df['test_type'].unique()
    
    # Create a 2x2 subplot grid (sharing axes makes comparison easier)
    fig, axes = plt.subplots(2, 2, figsize=(14, 10), sharex=True, sharey=True)
    axes = axes.flatten() # Flatten 2D array to 1D for easy iteration

    # Define consistent order if possible
    preferred_order = ['static', 'angle', 'lighting', 'occlusion']
    # Filter to only include tests that actually exist in the dataframe
    tests_to_plot = [t for t in preferred_order if t in test_types]
    
    # If we have extra tests not in preferred list, add them
    for t in test_types:
        if t not in tests_to_plot:
            tests_to_plot.append(t)

    for i, test in enumerate(tests_to_plot):
        if i >= len(axes): break # Safety check
        
        ax = axes[i]
        subset = df[df['test_type'] == test]
        
        # Plot Frame Times
        # We use alpha=0.9 to make lines solid and clear
        sns.lineplot(data=subset, x='frame_id', y='perf_time_ms', 
                     hue='method', palette=COLORS, 
                     ax=ax, linewidth=1.5)

        # Styling per subplot
        ax.set_title(f"Scenario: {test.capitalize()}", fontsize=12, fontweight='bold')
        ax.set_ylabel("Frame Time (ms)")
        ax.set_xlabel("Frame Index")
        ax.grid(True, linestyle=':', alpha=0.6)
        
        # Add a horizontal line for 33ms (approx 30 FPS target)
        ax.axhline(33.3, color='gray', linestyle='--', alpha=0.5, linewidth=1)
        if i == 0:
            ax.text(0, 35, "30 FPS Target", color='gray', fontsize=8)

        # Handle Legend: Only show it on the first plot to avoid clutter
        if i == 0:
            ax.legend(title="Method", loc='upper right')
        else:
            ax.get_legend().remove()

    plt.suptitle('Computational Performance Comparison (Lower is Better)', fontsize=16)
    plt.tight_layout()
    
    path = os.path.join(OUTPUT_DIR, "1_performance_all.png")
    plt.savefig(path, dpi=DPI)
    plt.close()
    print(f"Saved: {path}")

def plot_stability_comparison(df):
    """ 2. NFT vs Checkerboard for Translation and Rotation (Static Test Only) """
    # Filter for only the static stability test
    static_df = df[df['test_type'] == 'static'].copy()
    
    if static_df.empty:
        print("No static test data found for stability plot.")
        return

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)

    # --- Top: Translation ---
    for method in ['checkerboard', 'nft']:
        subset = static_df[static_df['method'] == method]
        subset = subset[subset['stab_trans_jitter'].notnull()]
        
        ax1.plot(subset['frame_id'], subset['stab_trans_jitter'], 
                 label=method.capitalize(), color=COLORS[method], alpha=0.8, linewidth=1.5)

    ax1.set_title('Pose Stability: Translation Jitter', fontsize=14)
    ax1.set_ylabel('Euclidean Distance (units)', fontsize=12)
    ax1.set_ylim(0, 2.5)  # <-- Limit y-axis to 2.5
    ax1.legend(loc='upper right')
    ax1.grid(True, linestyle='--', alpha=0.5)

    # --- Bottom: Rotation ---
    for method in ['checkerboard', 'nft']:
        subset = static_df[static_df['method'] == method]
        subset = subset[subset['stab_rot_jitter_rad'].notnull()]
        
        ax2.plot(subset['frame_id'], subset['stab_rot_jitter_rad'], 
                 label=method.capitalize(), color=COLORS[method], alpha=0.8, linewidth=1.5)

    ax2.set_title('Pose Stability: Rotation Jitter', fontsize=14)
    ax2.set_xlabel('Frame Index', fontsize=12)
    ax2.set_ylabel('Geodesic Angle (radians)', fontsize=12)
    ax2.grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, "2_stability_comparison.png")
    plt.savefig(path, dpi=DPI)
    plt.close()
    print(f"Saved: {path}")

def plot_robustness_gantt(df):
    """ 3. Gantt/Step chart for Occlusion, Lighting, Angle """
    tests = ['occlusion', 'lighting', 'angle']
    
    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
    
    for i, test in enumerate(tests):
        ax = axes[i]
        test_data = df[df['test_type'] == test]
        
        if test_data.empty:
            ax.text(0.5, 0.5, "No Data", ha='center', va='center')
            continue

        # Helper to calculate intervals
        def get_intervals(data):
            intervals = []
            start = None
            # Ensure sorted by frame_id
            data = data.sort_values('frame_id')
            
            for _, row in data.iterrows():
                fid = row['frame_id']
                is_success = row['success']
                
                if is_success:
                    if start is None:
                        start = fid
                else:
                    if start is not None:
                        intervals.append((start, fid - start))
                        start = None
            
            # Close last interval if active
            if start is not None:
                last_fid = data.iloc[-1]['frame_id']
                intervals.append((start, last_fid - start + 1))
            return intervals

        # Plot Checkerboard Track (Top)
        chess_data = test_data[test_data['method'] == 'checkerboard']
        if not chess_data.empty:
            intervals = get_intervals(chess_data)
            if intervals:
                # y=10, height=8
                ax.broken_barh(intervals, (10, 8), facecolors=COLORS['checkerboard'], label='Checkerboard')

        # Plot NFT Track (Bottom)
        nft_data = test_data[test_data['method'] == 'nft']
        if not nft_data.empty:
            intervals = get_intervals(nft_data)
            if intervals:
                # y=0, height=8
                ax.broken_barh(intervals, (0, 8), facecolors=COLORS['nft'], label='NFT')

        # Formatting
        ax.set_ylim(-5, 25)
        ax.set_yticks([4, 14])
        ax.set_yticklabels(['NFT', 'Checkerboard'])
        ax.set_title(f'Robustness Test: {test.capitalize()}', fontsize=12, fontweight='bold')
        ax.grid(True, axis='x', linestyle='--', alpha=0.5)
        
        # Add track boundaries for visual clarity
        min_f = test_data['frame_id'].min()
        max_f = test_data['frame_id'].max()
        ax.hlines([0, 8], xmin=min_f, xmax=max_f, colors='gray', linestyles=':', alpha=0.3)
        ax.hlines([10, 18], xmin=min_f, xmax=max_f, colors='gray', linestyles=':', alpha=0.3)

    axes[-1].set_xlabel('Frame Index', fontsize=12)
    plt.suptitle("Detection Robustness (Tracking Intervals)", fontsize=16)
    plt.tight_layout()
    
    path = os.path.join(OUTPUT_DIR, "3_robustness_gantt.png")
    plt.savefig(path, dpi=DPI)
    plt.close()
    print(f"Saved: {path}")

def main():
    ensure_dir(OUTPUT_DIR)

    # Define the file map
    # Structure: (FilePath, TestType, Method)
    files = [
        # Checkerboard
        ("data/statistics/Checkerboard/detection_robustness/session_stats_checkerboard_angle.json", "angle", "checkerboard"),
        ("data/statistics/Checkerboard/detection_robustness/session_stats_checkerboard_lighting.json", "lighting", "checkerboard"),
        ("data/statistics/Checkerboard/detection_robustness/session_stats_checkerboard_occlusion.json", "occlusion", "checkerboard"),
        ("data/statistics/Checkerboard/pose_stability/session_stats_checkerboard.json", "static", "checkerboard"),
        # NFT
        ("data/statistics/NFT/detection_robustness/session_stats_nft_angle.json", "angle", "nft"),
        ("data/statistics/NFT/detection_robustness/session_stats_nft_lighting.json", "lighting", "nft"),
        ("data/statistics/NFT/detection_robustness/session_stats_nft_occlusion.json", "occlusion", "nft"),
        ("data/statistics/NFT/pose_stability/session_stats_nft.json", "static", "nft"),
    ]

    dfs = []
    for path, test, method in files:
        df = load_json_df(path, test, method)
        if df is not None:
            dfs.append(df)

    if not dfs:
        print("No valid data files loaded. Check paths.")
        return

    # Merge all into one big dataframe
    all_data = pd.concat(dfs, ignore_index=True)

    # Generate the 3 requested plots
    plot_performance_all(all_data)
    plot_stability_comparison(all_data)
    plot_robustness_gantt(all_data)

    print("\nProcessing Complete.")

if __name__ == "__main__":
    main()