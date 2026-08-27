import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def process_depth_telemetry(file_path):
    print(f"Loading dataset from: {file_path}")
    
    # Load dataset & strip hidden spaces from column names
    df = pd.read_csv(file_path)
    df.columns = df.columns.str.strip()
    
    # Identify depth and point columns dynamically
    depth_col = [col for col in df.columns if 'depth' in col.lower()][0]
    point_col = [col for col in df.columns if 'point' in col.lower()][0]

    # Convert values to numeric (coercing #VALUE! to NaN)
    df['Raw_Numeric'] = pd.to_numeric(df[depth_col], errors='coerce')
    df['Depth_Cleaned'] = df['Raw_Numeric'].copy()

    # Filter out 0.0 m dropouts and extreme spikes below -800 m
    df.loc[df['Depth_Cleaned'] == 0, 'Depth_Cleaned'] = np.nan
    df.loc[df['Depth_Cleaned'] < -800, 'Depth_Cleaned'] = np.nan

    # Linear interpolation & 5-point moving average filter
    df['Depth_Interpolated'] = df['Depth_Cleaned'].interpolate(method='linear')
    df['Depth_Smoothed'] = df['Depth_Interpolated'].rolling(window=5, min_periods=1).mean()

    # ==========================================================================
    # VISUALIZATION 1: OVERLAY COMPARISON PLOT
    # ==========================================================================
    plt.figure(figsize=(10, 5))
    plt.plot(df[point_col], df['Raw_Numeric'], label='Raw Telemetry Data (Spikes/Noise)', color='#d9534f', alpha=0.5, linestyle='--')
    plt.plot(df[point_col], df['Depth_Smoothed'], label='Filtered Seabed Profile (5-Pt Moving Avg)', color='#0275d8', linewidth=2.5)
    
    plt.title('Avionics Task 1: Seafloor Depth Telemetry Processing', fontsize=12, fontweight='bold')
    plt.xlabel('Data Point Index', fontsize=10)
    plt.ylabel('Depth (m)', fontsize=10)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(loc='lower left', fontsize=10)
    plt.tight_layout()
    plt.savefig('raw_graph.png', dpi=300)
    plt.close()

    # ==========================================================================
    # VISUALIZATION 2: SIDE-BY-SIDE DETAILED COMPARISON
    # ==========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 4.5), sharey=True)

    ax1.plot(df[point_col], df['Raw_Numeric'], color='#d9534f', linewidth=1.2)
    ax1.set_title('Raw Telemetry (Unfiltered)', fontsize=11, fontweight='bold')
    ax1.set_xlabel('Point Index')
    ax1.set_ylabel('Depth (m)')
    ax1.grid(True, linestyle=':', alpha=0.6)

    ax2.plot(df[point_col], df['Depth_Smoothed'], color='#5cb85c', linewidth=2)
    ax2.set_title('Cleaned & Filtered Seabed Topology', fontsize=11, fontweight='bold')
    ax2.set_xlabel('Point Index')
    ax2.grid(True, linestyle=':', alpha=0.6)

    plt.tight_layout()
    plt.savefig('filtered_graph.png', dpi=300)
    plt.close()

    print("SUCCESS: 'raw_graph.png' and 'filtered_graph.png' generated cleanly!")

if __name__ == '__main__':
    # Auto-detect your CSV file name sitting in the folder
    csv_files = [f for f in os.listdir('.') if f.endswith('.csv')]
    if csv_files:
        process_depth_telemetry(csv_files[0])
    else:
        print("ERROR: No CSV file found in folder!")