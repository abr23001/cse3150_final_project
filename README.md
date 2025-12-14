# BGP Simulator

A BGP (Border Gateway Protocol) routing simulator that models Internet AS (Autonomous System) relationships and implements Route Origin Validation (ROV) security features.

---

### Build Command:

# Step 1: Compile src file to .0 files
```bash
  g++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude -c src/ASGraph.cpp -o src/ASGraph.o
  
  g++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude -c src/BGP.cpp -o src/BGP.o
  
  g++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude -c src/Announcement.cpp -o src/Announcement.o
  
  g++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude -c src/bgp_simulator.cpp -o src/bgp_simulator.o
```
# Step 2: Link all o. files into  ./bgp_simulator
```bash
  g++ -std=c++17 -Wall -Wextra -O0 -g -Iinclude src/ASGraph.cpp src/BGP.cpp src/Announcement.cpp src/bgp_simulator.cpp -o bgp_simulator
```
#Step 3: to run:
```bash
  ./bgp_simulator --relationships bench/subprefix/CAIDAASGraphCollector_2025.10.16.txt --announcements bench/subprefix/anns.csv --rov-asns bench/subprefix/rov_asns.csv
 ```
 
## Design Choices

### Core Architecture
- **Policy-Based Design**: Each AS uses either a BGP or ROV policy for processing routes.
- **Modern C++17**: Utilizes smart pointers (`std::unique_ptr`, `std::shared_ptr`) for automatic memory management.
- **Efficient Data Structures**: Hash maps and sets allow O(1) operations across large-scale networks.

### BGP Implementation
- **Correct CAIDA Parsing**  
  Relationship codes:
  - `-1`: Provider-to-Customer (AS1 → AS2)  
  - `0`: Peer-to-Peer  
  - `1`: Sibling (treated the same as peer)

- **Realistic BGP Decision Process**  
  Routes are selected based on:
  1. **Relationship priority**  
     ORIGIN > CUSTOMER > PEER > PROVIDER  
  2. **AS-path length** (shorter is preferred)  
  3. **Next-hop ASN** (lower ASN wins ties)

- **Three-Phase Propagation**
  - **UP**: Customer → Provider  
  - **ACROSS**: Peer ↔ Peer (synchronous)  
  - **DOWN**: Provider → Customer  

### Network Topology
- **Cycle Detection**: Detects and aborts on invalid provider–customer loops.
- **Propagation Ranks**: Automatically builds a hierarchy from leaf ASes upward.
- **Loop Prevention**: Ensures announcements are not sent back toward their source.

### ROV (Route Origin Validation)
- **Selective Filtering**: ROV-enabled ASes drop announcements marked `rov_invalid = True`.
- **Security Enhancement**: Simulates hijack mitigation.
- **Backward Compatibility**: Non-ROV ASes accept all routes.

---

## How to Build and Run

### Build
```bash
make clean && make
```


### Run
```bash
./bgp_simulator --relationships <relationships_file> \
                --announcements <announcements_file> \
                --rov-asns <rov_asns_file>
```


### Test Datasets
```bash
./bgp_simulator --relationships bench/subprefix/CAIDAASGraphCollector_2025.10.16.txt \
                --announcements bench/subprefix/anns.csv \
                --rov-asns bench/subprefix/rov_asns.csv

./bgp_simulator --relationships bench/prefix/CAIDAASGraphCollector_2025.10.16.txt \
                --announcements bench/prefix/anns.csv \
                --rov-asns bench/prefix/rov_asns.csv

./bgp_simulator --relationships bench/many/CAIDAASGraphCollector_2025.10.16.txt \
                --announcements bench/many/anns.csv \
                --rov-asns bench/many/rov_asns.csv
```


### Example Output for many:
```bash
abdullah@Acer-Predator:~/cse_3150/project3$ ./bgp_simulator --relationships ../bench/many/CAIDAASGraphCollector_2025.10.16.txt --announcements ../bench/many/anns.csv --rov-asns ../bench/many/rov_asns.csv
Loading AS relationships from: ../bench/many/CAIDAASGraphCollector_2025.10.16.txt
Loaded 78370 nodes in 507ms
Checking for cycles in AS relationships...
No cycles detected. Topology is valid.
Flattening graph for BGP propagation...
Graph flattened into 76 ranks in 135ms
Loading ROV-enabled ASNs from: ../bench/many/rov_asns.csv
Loaded 19 ROV-enabled ASNs
Initializing BGP policies...
Loading announcements from: ../bench/many/anns.csv
Seeded 40 announcements
Propagating BGP announcements...
BGP propagation completed in 17160ms
Writing results to: ribs.csv
Simulation complete!
Total routes in all RIBs: 2963832
```


### Output Format
The simulator generates a ribs.csv file containing the final routing tables:
```csv
asn,prefix,as_path
27,1.2.0.0/16,"(27,)"
25,1.2.3.0/24,"(25,)"
2152,1.2.3.0/24,"(2152, 25)"
10886,1.2.0.0/16,"(10886, 27)"
```


### Comparing Results
```bash
# Compare your output with expected results
./compare_output.sh bench/subprefix/ribs.csv ribs.csv
./compare_output.sh bench/prefix/ribs.csv ribs.csv
./compare_output.sh bench/many/ribs.csv ribs.csv
```

### Output from Compare on many
```bash
abdullah@Acer-Predator:~/cse_3150/project3$ ./compare_output.sh bench/many/ribs.csv ribs.csv
Comparing files (sorted, ignoring whitespace):
  Expected: bench/many/ribs.csv
  Actual:   ribs.csv

✓ Files match perfectly!
```

### Performance:
- Handles **78,370 ASNs** with **76 propagation ranks**
- Processes up to **2.96 million routes** (many dataset)
- BGP Simulator in many dataset runs in ~16 seconds

### Error Handling
The simulator exits with error code 1 and displays a message if:
- Provider-customer cycles are detected in the AS relationships
- Required input files cannot be opened
- Command-line arguments are missing
