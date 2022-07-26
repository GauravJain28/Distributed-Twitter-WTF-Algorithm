#include <mpi.h>
#include <bits/stdc++.h>
#include "randomizer.hpp"

using namespace std;

// support functions
void print_vector(pair<int*, int*>  v, int n);
void inputDat(string fname, bool isDirected, int m, vector<vector<int>>* graph, vector<int>* out_deg);
void writeDat(string fname, int n, int num_rec, pair<int*,int*>* rec, vector<int>* out_deg);

// comparator for sorting 
inline bool compare(const pair<int,int>& a, const pair<int,int>& b){
    return (a.first > b.first) || (a.first == b.first && a.second<b.second);
}

int main(int argc, char* argv[]){
    // input parameters
    // auto st = std::chrono::steady_clock::now();

    assert(argc > 8);
    std::string graph_file = argv[1];
    int num_nodes = std::stoi(argv[2]);
    int num_edges = std::stoi(argv[3]);
    float restart_prob = std::stof(argv[4]);
    int num_steps = std::stoi(argv[5]);
    int num_walks = std::stoi(argv[6]);
    int num_rec = std::stoi(argv[7]);
    int seed = std::stoi(argv[8]);

    // data structures to store graph, out degree and recommendations
    vector<vector<int>> graph(num_nodes);
    vector<int> out_deg(num_nodes,0);
    pair<int*,int*> rec[num_nodes];

    // input from edges.dat file
    inputDat(graph_file,1,num_edges,&graph,&out_deg);
    
    //Only one randomizer object should be used per MPI rank, and all should have same seed
    Randomizer random_generator(seed, num_nodes, restart_prob);

    int rank; // MPI Rank
    int size; // Total number of Ranks

    //Starting MPI pipeline
    MPI_Init(NULL, NULL);
    
    // Extracting Rank and Processor Count
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // process a subset of all nodes according to rank and size
    int a;                              // lower bound
    int b;                              // upper bound
    if(rank==size-1){                   // last rank
        a = rank*(num_nodes/size);
        b = num_nodes;
    }
    else{
        a = rank*(num_nodes/size);
        b = (rank+1)*(num_nodes/size);
    }
        
    for(int u=a;u<b;u++){
        // calculate recommendations
        rec[u].first = new int[num_rec];
        rec[u].second = new int[num_rec];

        //init recommendations
        for(int i=0;i<num_rec;i++){
            rec[u].first[i] = -1;
            rec[u].second[i] = -1;
        }

        //score keeper
        map<int,int> score;

        for(int v : graph[u]){
            for(int i=0;i<num_walks;i++){
                // RWR
                int node = v;
                int next_node; // randomly select next_node- move to neighbour node or restart at start node
                int num_child; // number of children of current node

                for(int i=0;i<num_steps;i++){

                    num_child = graph[node].size();
                    if(num_child>0){
                        // randomly selecting next node
                        int next_step = random_generator.get_random_value(u);
                        if(next_step < 0) // restart
                            next_node = v;
                        else              // neighbour of current node
                            next_node = graph[node][next_step%num_child];
                    }
                    else{
                        // no child to move forward -- restart
                        next_node = v;
                    }

                    node = next_node; // state next_node as current node
                    score[node]++;
                }
            }
        }

        // Make scores of start node and its neighbour 0 
        // As they are not included in recommendation
        score[u] = 0;
        for(int v : graph[u]) 
            score[v] = 0;

        
        // Sort scores
        vector<pair<int,int>> score_;
        for(auto x : score)
            score_.push_back({x.second,x.first});

        
        sort(score_.begin(),score_.end(),compare);

        // Generate recommended nodes
        int w = min(num_rec,(int)score_.size());
        for(int i=0;i<w;i++){ 
            //cout<<score_[i].first<<" "<<score_[i].second<<"\n";
            if(score_[i].first > 0){
                rec[u].first[i] = score_[i].second;
                rec[u].second[i] = score_[i].first;
            }
        }

        // cout<<u<<","<<out_deg[u]<<",";
        // print_vector(rec[u],num_rec);
    }

    // fix?
    if(rank==0){
        // collect all recommendations from other ranks
        // MPI_Recv(buffer, buffer_len, MPI_CHAR, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for(int i=b; i<num_nodes; i++){
            rec[i].first = new int[num_rec];
            rec[i].second = new int[num_rec];

            //identify sender rank, tag is node number whose recommedation is recieved
            int rr = (i/b);
            if(rr == size) rr = size-1;

            MPI_Recv(rec[i].first, num_rec, MPI_INT, rr, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(rec[i].second, num_rec, MPI_INT, rr, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    else{
        // send to rank 0
        for(int u=a;u<b;u++){
            MPI_Send(rec[u].first, num_rec, MPI_INT, 0, u, MPI_COMM_WORLD);
            MPI_Send(rec[u].second, num_rec, MPI_INT, 0, u, MPI_COMM_WORLD);
        }
    }
    //fix?

    // only rank 0 should write
    if(rank==0){
        string fname =  "output.dat";  // "my_" + to_string(num_steps)+"_" + to_string(num_walks) + "_" + to_string(num_rec) + ".dat";
        writeDat(fname,num_nodes,num_rec,rec,&out_deg);
    }
    
    MPI_Finalize();

    // auto end = std::chrono::steady_clock::now();
    // std::chrono::duration<double> elapsed_seconds = end-st;
    // std::cout << "Rank: " << rank << " " << "elapsed time: " << elapsed_seconds.count() << "s\n";
}



// input graph from .dat file
void inputDat(string fname, bool isDirected, int m, vector<vector<int>>* graph, vector<int>* out_deg){

    vector<vector<int>>& g = *graph;
    vector<int>& od = *out_deg;
    
    unsigned char buffer[4];
    int src;
    int dst;
    int count;
	FILE * fp;
    fp = fopen(fname.c_str(),"rb");

    while(m--){

        
        count = fread(&buffer,sizeof(char),4,fp);
        src = (int)buffer[3] | (int)buffer[2]<<8 | (int)buffer[1]<<16 | (int)buffer[0]<<24;
    
        count = fread(&buffer,sizeof(char),4,fp);
        dst = (int)buffer[3] | (int)buffer[2]<<8 | (int)buffer[1]<<16 | (int)buffer[0]<<24;
    

        //cout<<src<<" "<<dst<<"\n";

        if(isDirected){
            g[src].push_back(dst);
        }
        else{
            g[src].push_back(dst);
            g[src].push_back(dst);
        }

        od[src]++;
    }

    fclose(fp);
}

// output recommendations in a .dat file
void writeDat(string fname, int n, int num_rec, pair<int*,int*>* rec, vector<int>* out_deg){
    
    vector<int>& od = *out_deg;

    FILE * fp;
    fp = fopen(fname.c_str(),"wb+");

    unsigned char buffer[4];

    for(int i=0;i<n;i++){

        //if(rec[i].size()==0) continue;  // temporary pass

        int deg = od[i];

        buffer[3] =  deg        & 0xff;
        buffer[2] = (deg >> 8)  & 0xff;
        buffer[1] = (deg >> 16) & 0xff;
        buffer[0] = (deg >> 24) & 0xff;

        fwrite(&buffer,sizeof(buffer),1,fp);

        for(int x = 0; x<num_rec;x++){
            if(rec[i].first[x] == -1 && rec[i].second[x] == -1){
                // write NULL NULL
                fwrite("NULL",4,1,fp);
                fwrite("NULL",4,1,fp);
            }
            else{
                buffer[3] =  rec[i].first[x]        & 0xff;
                buffer[2] = (rec[i].first[x] >> 8)  & 0xff;
                buffer[1] = (rec[i].first[x] >> 16) & 0xff;
                buffer[0] = (rec[i].first[x] >> 24) & 0xff;

                fwrite(&buffer,sizeof(buffer),1,fp);

                buffer[3] =  rec[i].second[x]        & 0xff;
                buffer[2] = (rec[i].second[x] >> 8)  & 0xff;
                buffer[1] = (rec[i].second[x] >> 16) & 0xff;
                buffer[0] = (rec[i].second[x] >> 24) & 0xff;

                fwrite(&buffer,sizeof(buffer),1,fp);
            }
        }
    }

    fclose(fp);
}

void print_vector(pair<int*, int*>  v, int n){
    for(int i=0;i<n;i++)
        cout<<v.first[i]<<" "<<v.second[i]<<",";
    cout<<"\n";
}