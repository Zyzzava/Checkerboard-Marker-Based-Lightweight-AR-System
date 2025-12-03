import json
import os
import matplotlib.pyplot as plt
from matplotlib import table

# --- Configuration ---
OUTPUT_DIR = "data/figures"
DPI = 300

# --- Palette ---
C_HEADER_BG = "#2c3e50"      # Midnight Blue
C_HEADER_TXT = "#ffffff"     # White
C_ROW_EVEN = "#ffffff"       # White
C_ROW_ODD = "#f8f9fa"        # Very Light Gray
C_WIN_BG = "#d4edda"         # Pastel Green (Bootstrap Success)
C_WIN_TXT = "#155724"        # Dark Green
C_TEXT_MAIN = "#212529"      # Dark Gray
C_BORDER = "#dee2e6"         # Light Border Gray

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def load_summary(filepath):
    """ Loads just the summary section of the JSON """
    if not os.path.exists(filepath):
        return None
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)
        return data.get("summary", {})
    except:
        return None

def get_comparison_data():
    """ 
    Aggregates data into a structured list for the table.
    Returns a list of rows: [Category, Test, MetricName, ChessVal, NFTVal, BetterDirection]
    """
    
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

    raw_data = {} 

    for path, test, method in files:
        summary = load_summary(path)
        if not summary: continue
        
        if test not in raw_data: raw_data[test] = {}
        if method not in raw_data[test]: raw_data[test][method] = {}

        perf = summary.get("performance", {})
        raw_data[test][method]['time'] = perf.get("mean_frame_time_ms", 0.0)

        rob = summary.get("robustness", {})
        raw_data[test][method]['success'] = rob.get("success_rate", 0.0)

        stab = summary.get("pose_stability", {})
        raw_data[test][method]['trans_jitter'] = stab.get("translation_mean_error", 0.0)
        raw_data[test][method]['rot_jitter'] = stab.get("rotation_mean_error_rad", 0.0)

    table_rows = []

    # Section 1: Performance
    test_order = ['static', 'angle', 'lighting', 'occlusion']
    for t in test_order:
        if t in raw_data:
            c_val = raw_data[t].get('checkerboard', {}).get('time', None)
            n_val = raw_data[t].get('nft', {}).get('time', None)
            # Added arrow to label
            table_rows.append(["Performance", t.capitalize(), "Mean Frame Time (ms) $\downarrow$", c_val, n_val, 'min'])

    # Section 2: Robustness
    for t in ['angle', 'lighting', 'occlusion']:
        if t in raw_data:
            c_val = raw_data[t].get('checkerboard', {}).get('success', None)
            n_val = raw_data[t].get('nft', {}).get('success', None)
            table_rows.append(["Robustness", t.capitalize(), "Success Rate $\\uparrow$", c_val, n_val, 'max'])

    # Section 3: Stability
    if 'static' in raw_data:
        c_val_t = raw_data['static'].get('checkerboard', {}).get('trans_jitter', None)
        n_val_t = raw_data['static'].get('nft', {}).get('trans_jitter', None)
        table_rows.append(["Stability", "Static", "Mean Trans. Jitter (mm) $\downarrow$", c_val_t, n_val_t, 'min'])
        
        c_val_r = raw_data['static'].get('checkerboard', {}).get('rot_jitter', None)
        n_val_r = raw_data['static'].get('nft', {}).get('rot_jitter', None)
        table_rows.append(["Stability", "Static", "Mean Rot. Jitter (rad) $\downarrow$", c_val_r, n_val_r, 'min'])

    return table_rows

def create_table_figure():
    ensure_dir(OUTPUT_DIR)
    rows = get_comparison_data()
    
    if not rows:
        print("No data found to create table.")
        return

    # --- Pre-processing for Visuals ---
    cell_text = []
    
    # Logic to handle grouping visually
    prev_category = None
    
    # Store styling info: (row_index, col_index) -> (bg_color, text_color, weight)
    cell_styles = {} 

    col_labels = ["Category", "Test Scenario", "Metric", "Checkerboard", "NFT"]

    for i, row in enumerate(rows):
        cat, test, metric, c_val, n_val, opt = row
        
        # Determine Row Background (Zebra Striping)
        row_bg = C_ROW_EVEN if i % 2 == 0 else C_ROW_ODD

        # Format Values
        c_str = "N/A"
        n_str = "N/A"
        
        # Helper to format
        def fmt(val, m):
            if val is None: return "N/A"
            if "Success" in m: return f"{val*100:.1f}%"
            if "rad" in m: return f"{val:.4f}"
            return f"{val:.2f}"

        c_str = fmt(c_val, metric)
        n_str = fmt(n_val, metric)

        # Determine Winner
        c_bg, c_txt_col = row_bg, C_TEXT_MAIN
        n_bg, n_txt_col = row_bg, C_TEXT_MAIN
        c_weight, n_weight = 'normal', 'normal'

        if c_val is not None and n_val is not None:
            is_c_better = False
            if opt == 'min' and c_val < n_val: is_c_better = True
            elif opt == 'max' and c_val > n_val: is_c_better = True
            elif c_val == n_val: is_c_better = None # Tie

            if is_c_better is True:
                c_bg = C_WIN_BG
                c_txt_col = C_WIN_TXT
                c_weight = 'bold'
            elif is_c_better is False:
                n_bg = C_WIN_BG
                n_txt_col = C_WIN_TXT
                n_weight = 'bold'
            # If tie, neither gets highlighted

        # Handle Category Grouping (Visual merge)
        display_cat = cat
        if cat == prev_category:
            display_cat = "" # Hide text to look like merged cell
        prev_category = cat

        cell_text.append([display_cat, test, metric, c_str, n_str])

        # Store styles for later application
        # Columns: 0=Cat, 1=Test, 2=Metric, 3=Chess, 4=NFT
        for col_idx in range(5):
            bg = row_bg
            txt = C_TEXT_MAIN
            weight = 'normal'
            
            # Specific styling for data columns
            if col_idx == 3: # Checkerboard
                bg, txt, weight = c_bg, c_txt_col, c_weight
            elif col_idx == 4: # NFT
                bg, txt, weight = n_bg, n_txt_col, n_weight
            
            # Make Category Bold if it's the first time appearing
            if col_idx == 0 and display_cat != "":
                weight = 'bold'

            cell_styles[(i + 1, col_idx)] = (bg, txt, weight) # i+1 because row 0 is header

    # --- Plotting ---
    fig, ax = plt.subplots(figsize=(10, len(rows) * 0.6 + 1.5))
    ax.axis('off')

    # Create Table
    tab = ax.table(cellText=cell_text,
                   colLabels=col_labels,
                   loc='center',
                   cellLoc='center',
                   colWidths=[0.15, 0.15, 0.35, 0.175, 0.175])

    # --- Apply Advanced Styling ---
    
    # 1. Header Styling
    for col_idx in range(5):
        cell = tab[0, col_idx]
        cell.set_text_props(weight='bold', color=C_HEADER_TXT)
        cell.set_facecolor(C_HEADER_BG)
        cell.set_edgecolor(C_HEADER_BG) # Hide border
        cell.set_height(0.12) # Taller header
    
    # 2. Data Row Styling
    for (row_idx, col_idx), cell in tab.get_celld().items():
        if row_idx == 0: continue # Skip header

        bg, txt, weight = cell_styles.get((row_idx, col_idx), (C_ROW_EVEN, C_TEXT_MAIN, 'normal'))
        
        cell.set_facecolor(bg)
        cell.set_text_props(color=txt, weight=weight)
        cell.set_edgecolor(bg) # Hide individual cell borders for clean look
        cell.set_height(0.09)

        # Add a bottom border line for the whole row (simulated using edge color? No, draw manually)
        # Actually, let's keep it clean. Just a thin white divider or match the zebra.
        
        # Special visual trick: If category is empty, it merges with above. 
        # But we use Zebra striping, so we can't truly merge backgrounds easily if rows differ.
        # Instead, we just keep the text empty.

    # 3. Add Dividing Lines (Manual)
    # Under Header
    tab[0, 0].set_edgecolor(C_HEADER_BG)
    
    # Global Font Size
    tab.auto_set_font_size(False)
    tab.set_fontsize(11)

    # Title
    plt.title("Comparative Summary: Checkerboard vs NFT", 
              fontweight='bold', fontsize=16, color=C_HEADER_BG, y=0.98, pad=20)
    
    # Footnote
    plt.figtext(0.5, 0.05, "$\downarrow$ Lower is better | $\\uparrow$ Higher is better | Highlight indicates best performance", 
                ha="center", fontsize=9, color="#6c757d")

    filepath = os.path.join(OUTPUT_DIR, "4_summary_table.png")
    plt.savefig(filepath, dpi=DPI, bbox_inches='tight', pad_inches=0.3)
    plt.close()
    print(f"Saved pretty table to: {filepath}")

if __name__ == "__main__":
    create_table_figure()