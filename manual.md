Intersim说明书（英文）
the Manual of Intersim

# 1. Dependency
A. libzmq
B. cppzmq
C. boost

Additionally, cmake is used to build.

# 2. Module
## A. interchiplet
To forward messages between chiplet simulater instances.
## B. sniper
A chiplet simulater.
## C. interchiplet_app
A library for benchmark to communicate between chiplet simulater instances.
## D. record_combine
To transfer trace files.
## E. popnet
To calculate delay.
## F. power_calculate
To calculate power.

# 3. How to Build
Use the script "build.sh" in the catalogs of modules.

# 4. How to Run
## A. interchiplet
Enter the catalog "build" and create a text file "interchiplet.ini" as fellowing:
```
[config]
inter_address=tcp://*
port_base=8000
subnet_count=8
```
It means to open the ports in [port_base, port_base + subnet_count).
Run the executive file in the catalog "build".
## B. sniper
How many simulated chiplets, how many copies of sniper.
Create a text file "zmq_pro.ini" in the catalog "changes" as fellowing:
```
[config]
subnet_id=0
inter_address=tcp://localhost:8000
```
The port in the property "inter_address" should be equal to subnet_id + port_base.
Read sniper-mamual.pdf to get more information.
After running, run changes/build/record_transfer.
## C. interchiplet_app
Include the head file "interchiplet_app.h" and link the library file "libinerchiplet_app.a".
## D. record_combine
1) record_combine
It can combine trace files from different simulater instances. Run after copying "record_n.txt" from the catalogs of sniper instances. "n" is subnet_id.
Only one command line parameter "number of chiplets".

2) popnet_trace_transfer
It transfers the format of trace files.
3 parameters: chiplet count per line, core count per line in a chiplet and frequency by GHz.

3) random_phy_records
It produces random trace files, which need to be transfered by popnet_trace_transfer.
5 parameters: chiplet count, core count per chiplet, message count per period, period by ns and total time by ns.

## E. popnet
Just like original popnet. But c=4 and R=2 or 3. R=2 for mesh topology and R=3 for star topology between chiplets.

If you want to change delay per hop, change the function `void sim_router_template::flit_traversal(long i)` in the source file "sim_router.cc" and rebuild it.

## F. power_calculate
1) power_calculate
It is for mesh networks between chiplets.
3 parameters: trace file path, work per hop between chiplets and work per hop between cores.

2) star_topo_power_calculate
It is for star network between chiplets. It can only calculate counts of different hops.
1 parameter: trace file path.
Change `size_t chipletType(int64_t chipletX,int64_t chipletY)` to set different types of hops between chiplets.
