COMP 304 : Operating Systems
Project : Metro Simulation with POSIX Threads


How to run the project?

gcc -o <output_file> simulation.c -lpthread
./<output_file> <probability> -s <seconds>

The output file can accept two paremeters. First input is the probability used for train creation, it can be in interval [0.0, 1.0].Second input is  "-s" flag and simulation time in seconds.

Default situation: probability-> 0.5 and simulation time->60 seconds (When no command arguments entered)

This C program simulates a train tunnel control system using multi-threading. It dynamically generates trains at random intervals based on a specified probability. The train_arrival function manages train creation, while tunnel_control handles train passage through the tunnel, allowing one train at a time. The program features a logging system to track train activities and system events. It handles system overload by temporarily suspending new train arrivals when a certain limit is exceeded. Additionally, it includes functionality to simulate train breakdowns in the tunnel, adding realism to the simulation. The implementation showcases efficient use of pthreads for concurrency and mutexes for resource management in a simulated environment.

Implemented by Gülnisa Yıldırım 
