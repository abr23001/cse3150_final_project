# BGP Simulator with ROV (Route Origin Validation)

A comprehensive BGP routing simulator that models Internet AS (Autonomous System) relationships and implements Route Origin Validation (ROV) security features.

## Technologies Used

### Core Technologies
- **C++17**: Modern C++ with smart pointers and STL containers
- **Object-Oriented Design**: Policy pattern for BGP implementations (BGP, ROV)
- **Graph Theory**: AS topology flattening and cycle detection algorithms
- **Network Protocols**: BGP (Border Gateway Protocol) decision process simulation

### Key Components
- **Smart Pointers**: `std::shared_ptr` and `std::unique_ptr` for memory management
- **STL Containers**: `std::unordered_map`, `std::set`, `std::vector` for efficient data structures
- **Inheritance**: Abstract Policy class with BGP and ROV implementations
- **CSV/TSV Parsing**: Custom parsers handling various file formats and line endings

### Algorithms
- **Topological Sort**: Graph flattening for BGP propagation ranks
- **Cycle Detection**: DFS-based detection of provider/customer cycles
- **BGP Decision Process**: Multi-criteria route selection (relationship > path length > ASN)
- **Three-Phase Propagation**: Up→Across→Down announcement propagation model

## Build and Usage

### Building
```bash
make clean && make
```
This creates two executables:
- `asgraph` - Basic AS graph analysis
- `bgp_simulator` - Full BGP simulation with ROV

### Running the BGP Simulator

**From project root directory (recommended):**
```bash
# Large dataset (many ASes)
./bgp_simulator --relationships bench/many/CAIDAASGraphCollector_2025.10.16.txt --announcements bench/many/anns.csv --rov-asns bench/many/rov_asns.csv

# Prefix-focused dataset  
./bgp_simulator --relationships bench/prefix/CAIDAASGraphCollector_2025.10.15.txt --announcements bench/prefix/anns.csv --rov-asns bench/prefix/rov_asns.csv

# Subprefix dataset
./bgp_simulator --relationships bench/subprefix/CAIDAASGraphCollector_2025.10.15.txt --announcements bench/subprefix/anns.csv --rov-asns bench/subprefix/rov_asns.csv
```

**From a subdirectory:**
```bash
# If you want to run from a subdirectory, use ../bench/ paths:
mkdir results && cd results
../bgp_simulator --relationships ../bench/many/CAIDAASGraphCollector_2025.10.16.txt --announcements ../bench/many/anns.csv --rov-asns ../bench/many/rov_asns.csv
```

## Input File Formats

### AS Relationships File
**Format**: `AS1|AS2|relationship|source`
- `relationship = 0`: AS1 provides to AS2 (provider-to-customer)
- `relationship = -1`: AS1 is customer of AS2 (customer-to-provider)  
- `relationship = 1`: AS1 peers with AS2 (peer-to-peer)

**Example**:
```
1|11537|0|bgp
1|52640|-1|bgp
2|137661|1|bgp
```

### Announcements File
**Format**: `seed_asn,prefix,rov_invalid`
- `seed_asn`: ASN that originates the announcement
- `prefix`: IP prefix being announced (e.g., 10.0.0.0/24)
- `rov_invalid`: Boolean (True/False) indicating if announcement is ROV-invalid

**Example**:
```csv
seed_asn,prefix,rov_invalid
1,10.0.0.0/24,False
2,10.1.0.0/24,True
3,192.168.1.0/24,False
```

### ROV ASNs File
**Format**: Simple list of ASN numbers (one per line, no header)

**Example**:
```
174
1
2
3
```

## Output Files

### ribs.csv
**Format**: Tab-separated values with Python tuple-style AS-paths
```
asn	prefix	as_path
1	10.0.0.0/24	(1,)
2	10.0.0.0/24	(2, 52640, 1)
174	192.168.1.0/24	(174, 6939, 3)
```

**Columns**:
- `asn`: The AS storing this route in its RIB (Routing Information Base)
- `prefix`: The IP prefix/network being routed
- `as_path`: The AS-Path as a Python tuple showing the route through the Internet

## Comparing Results

### Using compare_output.sh
The comparison script compares your simulator output with expected results:

```bash
# Make script executable (first time only)
chmod +x compare_output.sh

# Compare with expected output
./compare_output.sh bench/many/ribs.csv ribs.csv
./compare_output.sh bench/prefix/ribs.csv ribs.csv
./compare_output.sh bench/subprefix/ribs.csv ribs.csv
```

**Script Features**:
- Sorts both files before comparison (handles different output orders)
- Ignores whitespace differences
- Provides clear success/failure messages
- Shows detailed diff output for debugging

**Example Usage**:
```bash
# Run simulation and compare
./bgp_simulator --relationships ../bench/many/CAIDAASGraphCollector_2025.10.15.txt --announcements ../bench/many/anns.csv --rov-asns ../bench/many/rov_asns.csv
./compare_output.sh bench/many/ribs.csv ribs.csv
```

## Key Features

### BGP Simulation
- **Realistic BGP Decision Process**: Implements standard BGP route selection criteria
- **Three-Phase Propagation**: Models real Internet routing behavior
- **AS-Path Construction**: Proper AS-path prepending during route propagation
- **Loop Prevention**: Avoids sending announcements back to their source

### ROV (Route Origin Validation)
- **Security Enhancement**: ROV-enabled ASes drop announcements marked as invalid
- **Selective Deployment**: Only configured ASes implement ROV filtering
- **Attack Mitigation**: Demonstrates how ROV prevents prefix hijacking attacks

### Topology Validation
- **Cycle Detection**: Identifies invalid provider/customer cycles
- **Hierarchy Enforcement**: Ensures Internet's hierarchical structure
- **Error Reporting**: Clear messages explaining topology violations

### Performance
- **Smart Pointers**: Efficient memory management without manual allocation
- **Optimized Data Structures**: Hash maps and sets for fast lookups
- **Scalable Design**: Handles networks with 100K+ nodes and 500K+ relationships

## Error Handling

### Common Error Messages
```bash
# Cycle Detection
"ERROR: Provider cycle detected in AS relationships!"
"ERROR: Customer cycle detected in AS relationships!"

# File Errors  
"Error opening file: filename.txt"
"Failed to load dataset"

# Usage Errors
"Usage: ./bgp_simulator --relationships <file> --announcements <file> --rov-asns <file>"
```

### Troubleshooting
1. **Cycle Errors**: The CAIDA datasets contain cycles, which is normal for real Internet data
2. **File Not Found**: Ensure file paths are correct and files exist
3. **Format Errors**: Check that input files match expected formats
4. **Memory Issues**: For very large datasets, monitor system memory usage

## Dataset Information

### Bench Directory Structure
```
bench/
├── many/                    # Large dataset
│   ├── CAIDAASGraphCollector_2025.10.15.txt
│   ├── CAIDAASGraphCollector_2025.10.16.txt  
│   ├── anns.csv
│   ├── rov_asns.csv
│   └── ribs.csv             # Expected output
├── prefix/                  # Prefix-focused dataset
│   ├── CAIDAASGraphCollector_2025.10.15.txt
│   ├── anns.csv
│   ├── rov_asns.csv
│   └── ribs.csv
└── subprefix/              # Subprefix dataset
    ├── CAIDAASGraphCollector_2025.10.15.txt
    ├── anns.csv
    ├── rov_asns.csv
    └── ribs.csv
```

### Statistics
- **Nodes**: ~78,000 ASes
- **Relationships**: ~489,000 provider/customer links
- **Announcements**: Variable per dataset
- **ROV ASNs**: 19 ROV-enabled ASes across datasets

## Development

### Code Structure
```
include/
├── ASNode.h          # AS node with relationships and policy
├── ASGraph.h         # Main graph class with BGP functionality
├── Announcement.h    # BGP announcement structure
└── Policy.h          # Abstract policy with BGP/ROV implementations

src/
├── ASGraph.cpp       # Graph operations and BGP propagation
├── BGP.cpp          # BGP and ROV policy implementations
├── bgp_simulator.cpp # Main simulator executable
└── main.cpp         # Basic graph analysis tool
```

### Extending the Simulator
- Add new routing policies by inheriting from the Policy class
- Implement additional BGP features in the BGP class
- Extend announcement attributes in the Announcement class
- Add new output formats in the outputToCSV method

## References

- CAIDA AS Relationships Dataset
- RFC 4271: Border Gateway Protocol 4 (BGP-4)
- RFC 6811: BGP Prefix Origin Validation
- Internet Routing Architecture and Security Research