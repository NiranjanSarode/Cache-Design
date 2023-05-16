#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;
#include <fstream>
#include <queue>

// This function parses the input line and extracts the operation and address
void parse_input(const std::string &input, std::string &op, long long &addr)
{
    std::stringstream ss(input);
    ss >> op >> std::hex >> addr;
}

// This function converts a decimal number to binary
void decimaltobinary(long long line, long long binaryNum[])
{
    long long n = line;
    long long i = 0;
    while (i < 64)
    {
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }
}

// This function takes the binary logarithm of a number
long long log2(long long n)
{
    long long ans = 0;
    while (n > 1)
    {
        n = n / 2;
        ans++;
    }
    return ans;
}

// Main function
int main(int argc, char **argv)
{
    long long BLOCKSIZE = atoi(argv[1]);
    long long L1_SIZE = atoi(argv[2]);
    long long L1_ASSOC = atoi(argv[3]);
    long long L2_SIZE = atoi(argv[4]);
    long long L2_ASSOC = atoi(argv[5]);
    const char *path = argv[6];


    // initialise the cache
    struct CacheEntry
    {
        bool valid; // whether the cache block is valid or not
        bool dirty; // whether the cache block has been modified or not
        long long tag;    // the tag bits for the cache block
        long long value;  // the value stored in the cache block
    };

    long long L1_row = L1_SIZE / (L1_ASSOC * BLOCKSIZE);
    long long L2_row = L2_SIZE / (L2_ASSOC * BLOCKSIZE);
    long long L1_index_bits = log2(L1_row);
    long long L2_index_bits = log2(L2_row);

    CacheEntry *L1_cache[L1_row][L1_ASSOC];
    CacheEntry *L2_cache[L2_row][L2_ASSOC];

    // Initialise the LRU cache for L1 and L2
    vector<long long> L1_LRUCache[L1_row];
    for (long long i=0; i<L1_row; i++){
        for(long long j=0; j<L1_ASSOC; j++){
            L1_LRUCache[i].push_back(j);
        }
    }

    vector<long long> L2_LRUCache[L2_row];
    for (long long i=0; i<L2_row; i++){
        for(long long j=0; j<L2_ASSOC; j++){
            L2_LRUCache[i].push_back(j);
        }
    }

    for (long long i = 0; i < L1_SIZE / (L1_ASSOC * BLOCKSIZE); i++)
    {
        for (long long j = 0; j < L1_ASSOC; j++)
        {
            L1_cache[i][j] = new CacheEntry;
            L1_cache[i][j]->valid = false;
            L1_cache[i][j]->dirty = false;
            L1_cache[i][j]->tag = -1;
        }
    }
    for (long long i = 0; i < L2_SIZE / (L2_ASSOC * BLOCKSIZE); i++)
    {
        for (long long j = 0; j < L2_ASSOC; j++)
        {
            L2_cache[i][j] = new CacheEntry;
            L2_cache[i][j]->valid = false;
            L2_cache[i][j]->dirty = false;
            L2_cache[i][j]->tag = -1;
        }
    }

    // initialise the required parameters
    long long L1_read = 0;
    long long L1_read_miss = 0;
    long long L1_write = 0;
    long long L1_write_miss = 0;
    long long L1_writeback = 0;
    long long L2_read = 0;
    long long L2_read_miss = 0;
    long long L2_write = 0;
    long long L2_write_miss = 0;
    long long L2_writeback = 0;

    ifstream file(path);
    if (file.is_open())
    {
        string line;
        while (std::getline(file, line))
        {
            // initialise the temporary variables
            long long addr;
            string command;
            parse_input(line, command, addr);
            long long L1_offset = 0;
            long long L1_index = 0;
            long long L1_tag = 0;
            long long L2_offset = 0;
            long long L2_index = 0;
            long long L2_tag = 0; 
            long long binaryNum[64];
            long long blockbits = log2(BLOCKSIZE);
            decimaltobinary(addr, binaryNum);
            long long power = 1;
            for (long long i = blockbits; i < L1_index_bits + blockbits; i++)
            {
                L1_index = L1_index + binaryNum[i] * power;
                power = power * 2;
            }
            // Here L1_index has value of index bits in decimal
            power=1;
            for(long long i =  L1_index_bits + blockbits; i<64; i++){
                L1_tag = L1_tag + binaryNum[i] * power;
                power = power * 2;
            }
            // Here L1_tag has value of tag bits in decimal
            power=1;
            for (long long i = blockbits; i < L2_index_bits + blockbits; i++)
            {
                L2_index = L2_index + binaryNum[i] * power;
                power = power * 2;
            }
            // Here L2_index has value of index bits in decimal
            power=1;
            for(long long i =  L2_index_bits + blockbits; i<64; i++){
                L2_tag = L2_tag + binaryNum[i] * power;
                power = power * 2;
            }
            // Here L2_tag has value of tag bits in decimal


            bool L1_miss=false;

            if(command == "w"){
                L1_write++;
            }
            else{
                L1_read++;
            }

            // check if the block is present in L1 cache
            for (long long i=0; i<L1_ASSOC; i++){
                if(L1_cache[L1_index][i]->valid == true && L1_cache[L1_index][i]->tag == L1_tag){
                    // block is present in L1 cache
                    // update the LRU cache
                    auto it = std::find(L1_LRUCache[L1_index].begin(), L1_LRUCache[L1_index].end(), i);
                    if (it != L1_LRUCache[L1_index].end()) {
                        L1_LRUCache[L1_index].erase(it);
                        L1_LRUCache[L1_index].push_back(i);
                    } 
                    // set L1_miss to false
                    L1_miss=false;
                    // if command is write, set the dirty bit
                    if(command == "w"){
                        L1_cache[L1_index][i]->dirty = true;
                    }
                    break;
                }
                else if(L1_cache[L1_index][i]->valid == false || L1_cache[L1_index][i]->tag != L1_tag){
                    L1_miss=true;
                }   
            }

            // if block is not present in L1 cache, check in L2 cache
            if(L1_miss==true){

                if(command == "w"){
                    L1_write_miss++;
                }
                else{
                    L1_read_miss++;
                }

                // update the LRU cache for L2
                long long L1_evict_index = L1_LRUCache[L1_index][0];
                bool L1_checkdirty = L1_cache[L1_index][L1_evict_index]->dirty;
                long long oldtag = L1_cache[L1_index][L1_evict_index]->tag;


                // writeback to L2 if dirty bit is set
                if(L1_checkdirty){

                    L1_writeback++;
                    bool L2_wmiss=false;
                    power = 1;
                    // calculate the tag and index for L2
                    long long temp = L1_index_bits;
                    while(temp > 0){
                        power = power * 2;
                        temp--;
                    }
                    long long temp2 = oldtag * power + L1_index;
                    temp = L2_index_bits;
                    power=1;
                    while(temp > 0){
                        power = power * 2;
                        temp--;
                    }
                    long long tempindex = temp2 % power;
                    long long temptag = temp2 / power;
                    L2_write++;

                    
                    for(long long i=0; i<L2_ASSOC; i++){
                        // check if the block is present in L2 cache
                        if(L2_cache[tempindex][i]->valid == true && L2_cache[tempindex][i]->tag == temptag){
                            auto it = std::find(L2_LRUCache[tempindex].begin(), L2_LRUCache[tempindex].end(), i);
                            if (it != L2_LRUCache[tempindex].end()) {
                                L2_LRUCache[tempindex].erase(it);
                                L2_LRUCache[tempindex].push_back(i);
                            }
                            L2_wmiss=false;
                            L2_cache[tempindex][i]->dirty = true;
                            break;
                        }
                        else if(L2_cache[tempindex][i]->valid == false || L2_cache[tempindex][i]->tag != temptag){
                            L2_wmiss=true;
                        }
                    }
                    if(L2_wmiss==true){
                        // update the LRU cache for L2
                        L2_write_miss++;
                        long long L2_evictindex = L2_LRUCache[tempindex][0];
                        bool L2_checkdirty = L2_cache[tempindex][L2_evictindex]->dirty;
                        if(L2_checkdirty){
                            // writeback to memory
                            L2_writeback++;
                        }
                        L2_LRUCache[tempindex].erase(L2_LRUCache[tempindex].begin());
                        L2_LRUCache[tempindex].push_back(L2_evictindex);
                        L2_cache[tempindex][L2_evictindex]->tag = temptag;
                        L2_cache[tempindex][L2_evictindex]->valid = true;
                        L2_cache[tempindex][L2_evictindex]->dirty = true;
                    }
                }


                // first read L2 irrespective whether you want ot writ/read in L1
                L2_read++;
                bool L2_miss=false;
                for(long long i=0; i<L2_ASSOC; i++){
                    // check if the block is present in L2 cache
                    if(L2_cache[L2_index][i]->valid == true && L2_cache[L2_index][i]->tag == L2_tag){
                        auto it = std::find(L2_LRUCache[L2_index].begin(), L2_LRUCache[L2_index].end(), i);
                        if (it != L2_LRUCache[L2_index].end()) {
                            L2_LRUCache[L2_index].erase(it);
                            L2_LRUCache[L2_index].push_back(i);
                        }
                        L2_miss=false;
                        break;
                    }
                    else if(L2_cache[L2_index][i]->valid == false || L2_cache[L2_index][i]->tag != L2_tag){
                        L2_miss=true;
                    }
                }


                if(L2_miss==true){
                    // read from memory to L2
                    L2_read_miss++;
                    long long L2_evict_index = L2_LRUCache[L2_index][0];
                    bool L2_checkdirty = L2_cache[L2_index][L2_evict_index]->dirty;
                    if(L2_checkdirty){
                        // writeback to memory
                        L2_writeback++;
                    }
                    // update the LRU cache for L2
                    L2_LRUCache[L2_index].erase(L2_LRUCache[L2_index].begin());
                    L2_LRUCache[L2_index].push_back(L2_evict_index);
                    L2_cache[L2_index][L2_evict_index]->tag = L2_tag;
                    L2_cache[L2_index][L2_evict_index]->valid = true;
                    L2_cache[L2_index][L2_evict_index]->dirty = false;
                }


                // update LRU for L1
                L1_LRUCache[L1_index].erase(L1_LRUCache[L1_index].begin());
                L1_LRUCache[L1_index].push_back(L1_evict_index);
                L1_cache[L1_index][L1_evict_index]->tag = L1_tag;
                L1_cache[L1_index][L1_evict_index]->valid = true;

                // if command is write, set dirty bit to true else set false
                // we bring the block to L1 cache from L2 , so it is not dirty if read command
                if(command == "w"){
                    L1_cache[L1_index][L1_evict_index]->dirty = true;
                }
                else{
                    L1_cache[L1_index][L1_evict_index]->dirty = false;
                }
            }
        }
    }
    file.close();
    std::cout << "number of L1 reads : " << L1_read << std::endl;
    std::cout << "number of L1 read misses : " << L1_read_miss << std::endl;
    std::cout << "number of L1 writes : " << L1_write << std::endl;
    std::cout << "number of L1 write misses : " << L1_write_miss << std::endl;
    std::cout << "L1 miss rate : " << (float)(L1_read_miss + L1_write_miss) / (L1_read + L1_write) << std::endl;
    std::cout << "number of writebacks from L1 memory : " << L1_writeback << std::endl;
    std::cout << "number of L2 reads : " << L2_read << std::endl;
    std::cout << "number of L2 read misses : " << L2_read_miss << std::endl;
    std::cout << "number of L2 writes : " << L2_write << std::endl;
    std::cout << "number of L2 write misses : " << L2_write_miss << std::endl;
    std::cout << "L2 miss rate : " << (float)(L2_read_miss + L2_write_miss) / (L2_read + L2_write) << std::endl;
    std::cout << "number of writebacks from L2 memory : " << L2_writeback << std::endl;
    std::cout << "Time taken : " << (float)(L1_read + L1_write +  L2_read * 20 + L2_write * 20 + L2_read_miss*200+ L2_write_miss*200 + L2_writeback*220 + L1_writeback)/10000000  <<" (10ms) "  << std::endl;
    cout<<endl;
    return 0;
}