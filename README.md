# Distributed Twitter's Who To Follow Algorithm
This repo contains the code of the distributed implementation of Twitter's famous Who to Follow (WTF) Algorithm for recommendations using OpenMPI.  

This is the solution of Assignment 2 of the course COL380- Intro to Parallel and Distributed Programming offered in the Second (Holi) Semester 2021-22.

## WTF: The Who to Follow Service at Twitter
Twitter uses a variant of pagerank algorithm, Random Walk with restart(RWR). Its personalised page rank described below to compute a 'circle of trust' for recommending whom to follow.

If a user u follows v1 and v2, the recommended users in the circle of trust of u are computed by the RWR algorithm, starting with the hub, L = {v1, v2}. 

Given that Twitter graphs are massive graphs (over 20 billion edges), one needs parallel algorithms for doing such computations for every user node in the graph. Read more [here](./A2_PS.pdf).

## How to run the code?
1. To compile all files. Run:
```
make compile
```
2. To execute, modify ```makefile``` or execute on default values. Run: 
```
make run
```

## Scaling Experiments
The scaling results are calculated on 3 graphs present in the ```data``` folder. The results are present [here](./report.txt). 