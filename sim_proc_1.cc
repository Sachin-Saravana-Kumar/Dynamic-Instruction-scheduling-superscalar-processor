#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <cstdint>
#include "sim_proc.h"

FILE* FP= nullptr;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
    struct cycle_counter{
        int FE_AC;
        int FE_NC;
        int DE_AC;
        int DE_NC;
        int RN_AC;
        int RN_NC;
        int RR_AC;
        int RR_NC;
        int DI_AC;
        int DI_NC;
        int IS_AC;
        int IS_NC;
        int EX_AC;
        int EX_NC;
        int WB_AC;
        int WB_NC;
        int RT_AC;
        int RT_NC;
        
    };
    struct pipelining{
        int op_type;
        int dst;
        int rs1;
        int rs2;
        bool rob_src1;
        bool rob_src2;
        bool empty;
        int sl_no;
        cycle_counter values;
    };

    struct function_unit{
        int op_type;
        int dst;
        int rs1;
        int rs2;
        bool rob_src1;
        bool rob_src2;
        bool valid;
        int no_of_cycles;
        int sl_no;
        cycle_counter values;
    }; 

    struct RMT{
        int rob_tag;
        bool valid;
    };

    struct ROB{
        int dst;
        int rs1;
        int rs2;
        bool rdy;
        int pc;
        pipelining values;

    };


    struct print_st{
        int sl_no;
        int op_type;
        int dst;
        int rs1;
        int rs2;
        cycle_counter FE;
        cycle_counter DE;
        cycle_counter RN;
        cycle_counter RR;
        cycle_counter DI;
        cycle_counter IS;
        cycle_counter EX;
        cycle_counter WB;
        cycle_counter RT;
    };

    struct ISSUE_Q{
        bool valid;
        int op_type;
        int dst_tag;
        bool rs1_rdy;
        int rs1_tag;
        bool rs2_rdy;
        int rs2_tag;
        int seq;
        bool rob_src1;
        bool rob_src2;
        int sl_no;
        cycle_counter values;
    };


class out_of_order{
    public:
    int sl_no;
    int width;
    int rob_size;
    int iq_size;
    pipelining* DE;
    pipelining* RN;
    pipelining* RR;
    pipelining* DI;
    function_unit* FU;
    int FU_COUNT;
    int head;
    int tail;
    pipelining* WB;
    RMT* RMT_TABLE;
    ROB* ROB_TABLE;
    print_st* print_state;
    ISSUE_Q* IQ;
    int PC;
    int IQ_entries;
    int IQ_seq;
    bool iq_full;
    int fu_seq;
    int advance_cycle;
    int end_of_file;
    int end_of_stage;


    out_of_order(int w,int r,int q) : width(w), rob_size(r), iq_size(q){
        DE = new pipelining[width];
        RN = new pipelining[width];
        RR = new pipelining[width];
        DI = new pipelining[width];
        FU = new function_unit[width*5];
        WB = new pipelining[width*5];
        FU_COUNT = 0;
        fu_seq = 0;
        advance_cycle = 0;
        sl_no = 0;
        end_of_file = 0;
        end_of_stage = 0;
        for(int i = 0; i < width*5 ;i++){
                FU[i] = {-1,-1,-1,-1,0,0,0,-1,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
                WB[i] = {-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};

        }        

        RMT_TABLE = new RMT[67];
        ROB_TABLE = new ROB[rob_size];
        print_state =  new print_st[rob_size];
        IQ        = new ISSUE_Q[iq_size];
        head = 0;
        tail = 0;
        PC = 0;
        IQ_seq = 0;
        iq_full = false;
        IQ_entries = 0;
        for(int i =0; i < width; i++)
        {
            DE[i] ={-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
            RN[i] = {-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
            RR[i] = {-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
            DI[i] = {-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};

        }
        for(int i =0; i < 67; i++)
        {
            RMT_TABLE[i] = {-1,0};
        }
        for(int i =0; i < rob_size; i++)
        {
            ROB_TABLE[i] = {-1,-1,-1,0,-1,{-1, -1,-1,-1,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};

        }
        for(int i =0; i < iq_size; i++)
        {
            IQ[i] ={ 0,-1, -1,0,-1,0,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
        }
        
    }

    ~out_of_order(){

        delete[] DE;
        delete[] RN;
        delete[] RR;
        delete[] DI;
        delete[] IQ;
        delete[] RMT_TABLE;
        delete[] ROB_TABLE;
        delete[] FU;
        delete[] WB;
    }

    void fetch(){
    uint64_t pc;
    int op_type, dest, src1, src2;
    if(DE[0].empty == 0){
    for (int i=0; i < width;i++){
        fetch_count(DE[i].values);
        if(fscanf(FP, "%llx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF){
            DE[i] = {op_type, dest,src1,src2,false, false, true,sl_no,advance_cycle,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0};
            decode_count(DE[i].values);
            sl_no++;
            decode_count(DE[i].values);
        }
        else{
            for(int j = 0;j < 4;j++)
            DE[i] = {-1, -1,-1,-1,false, false, false,-1,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0};
            end_of_file = 1;
        }
        advance_cycle++;
    }
    }
    }
    void fetch_count(cycle_counter &fe){
        fe.FE_AC = advance_cycle;
        // if (fe.FE_AC == -1)
        // {
        //     fe.FE_AC = advance_cycle;
        //     fe.FE_AC = 1;
        // }
        // fe.FE_AC++;
    }
    void decode_count(cycle_counter &de){

        de.DE_AC = advance_cycle + 1;
        // if(de.DE_AC == -1){
        //     de.DE_AC = advance_cycle;
        //     de.DE_NC = 0;
        // }
        // de.DE_NC++;
        
    }


    void decode(){
        if(RN[0].empty == 0 && DE[0].empty == 1){
            //update_print(DE);
            switch_array(DE,RN);
            for (int i = 0; i < width; ++i) {
                RN[i].values.RN_AC = advance_cycle + 1;
            }
            RN[0].empty =1;
            DE[0].empty =0;
        }
    }


    void rename(){
                if(RR[0].empty == 0 && RN[0].empty == 1 && check_rob()){
                    rename_change(RN);
                    switch_array(RN,RR);
                    for (int i = 0; i < width; ++i) {
                        RR[i].values.RR_AC = advance_cycle + 1;
                    }
                    RR[0].empty = 1;
                    RN[0].empty = 0;
                }
                else if(RR[0].empty == 1  && RN[0].empty ==1){
                    // for (int i = 0; i < width; ++i) {
                    //     rename_counter(RN[i].values);
                    // }
                }
            }
    void rename_change(pipelining* RN){
        for(int i = 0; i < width; i++)
        {
            
            PC++;
            ROB_TABLE[tail].rs1 = RN[i].rs1;
            ROB_TABLE[tail].rs2 = RN[i].rs2;
            RN[i].rob_src1 = source_change(i,RN[i].rs1,1);
            RN[i].rob_src2 = source_change(i,RN[i].rs2,2);
                ROB_TABLE[tail].dst = RN[i].dst; // in rob table entering the dst in the tail 
                ROB_TABLE[tail].pc  = RN[i].sl_no; //saving program count
                RMT_TABLE[RN[i].dst] = {tail, 1};
                RN[i].dst = tail;
                inc_tail();
        }
    }

    bool source_change(int i , int value_rs, int k){
        if(value_rs != -1 &&  RMT_TABLE[value_rs].valid == 1){
            if(k == 1){
                
                RN[i].rs1 = RMT_TABLE[value_rs].rob_tag;

            }
            else {
                
                RN[i].rs2 = RMT_TABLE[value_rs].rob_tag;
            }
            return true;
        }
        return false;
    }

    void inc_tail(){
        tail++;
        if(tail == rob_size)
            tail = 0;
    }
    
    bool check_rob(){
        int tail_temp = tail;
        int empty_count = 0;
        if(head == tail && ROB_TABLE[tail].pc == -1){
            for(int i = 0; i < width ; i++){   
                tail_temp+= 1;
            if(ROB_TABLE[tail_temp].pc == -1){
                empty_count++;
            }
            if(tail_temp == head)
            {
                return false;
            }
        }
        if(empty_count == width)
            return true;
        }
        for(int i = 0; i < width ; i++)
        {   tail_temp+= 1;
            if(tail_temp == rob_size){
                tail_temp = 0;
            }
            if(tail_temp == head)
            {
                return false;
            }
        }
        return true;
    }

    void regread(){
        if(DI[0].empty == 0 && RR[0].empty == 1){
                    switch_array(RR,DI);
        for (int i = 0; i < width; ++i) {
             DI[i].values.DI_AC = advance_cycle + 1; 
        }
                    DI[0].empty =1;
                    RR[0].empty =0;
                }
    }

    void dispatch(){
        if((iq_size-IQ_entries) >= width && DI[0].empty == 1){
                for (int i = 0; i < width; ++i) {
                    DI[i].values.IS_AC = advance_cycle + 1;
                    DI[i].values.IS_NC = 1;
                }
                    get_issue();
                    DI[0].empty =0;
        }
    }

    void get_issue(){
        int k = 0;
        for(int i = 0;i < width;i++)
        {
            k = find_and_place_in_iq(i,k);
            IQ_seq++;
            IQ_entries++;

        }
    }

    int find_and_place_in_iq(int i,int k){
            for(int j = 0;j < iq_size; j++){
                if(IQ[j].valid == 0){
                    IQ[j].valid = true;
                    IQ[j].op_type = DI[i].op_type;
                    IQ[j].dst_tag = DI[i].dst;
                    IQ[j].rs1_rdy = !(DI[i].rob_src1);
                    IQ[j].rs1_tag = DI[i].rs1;
                    IQ[j].rs2_rdy = !(DI[i].rob_src2);
                    IQ[j].rs2_tag = DI[i].rs2;
                    IQ[j].seq = IQ_seq;
                    IQ[j].rob_src1 = DI[i].rob_src1;
                    IQ[j].rob_src2 = DI[i].rob_src2;
                    IQ[j].sl_no    = DI[i].sl_no;
                    IQ[j].values = DI[i].values;
                    if(k == iq_size){
                        return 0;
                    }
                    return j;
                }
            }
            
    }
    void issue(){
        if(((width*5) - FU_COUNT) >= width && IQ_entries >= width){
            push_values_to_ex();
            for (int i = 0; i < iq_size; ++i) {
                if(IQ[i].valid)
                        IQ[i].values.IS_NC++;
                    }
        }
    }

    void push_values_to_ex()
    {
        int min_iq_seq = findSmallest();
        int count = 0;
        int k = 0;
        for(int j = min_iq_seq;j < IQ_seq;j++){
            for(int i = 0;i < iq_size; i++){
                if(IQ[i].sl_no == 12){
                }
                if(IQ[i].seq == j && IQ[i].valid == 1 && IQ[i].rs1_rdy == 1 && IQ[i].rs2_rdy == 1 && count < width)
                {
                    IQ[i].values.EX_AC = advance_cycle + 1;
                    IQ[i].values.EX_NC = 1;
                    k = find_place_ex(i,k);
                    FU_COUNT++;
                    IQ_entries--;
                    count++;
                }
            }
        }
    }

    int find_place_ex(int i,int k){
        if(k == width*5){
            k = 0;
        }
        for(int j = k ; j < width*5 ; j++){
            if(FU[j].valid == false){
                    IQ[i].valid = false;
                    FU[j].op_type = IQ[i].op_type;
                    FU[j].valid = true;
                    FU[j].dst = IQ[i].dst_tag;
                    FU[j].rs1 = IQ[i].rs1_tag;
                    FU[j].rs2 = IQ[i].rs2_tag;
                    IQ[i].seq = 0;
                    FU[j].rob_src1 = IQ[i].rob_src1;
                    FU[j].rob_src2 = IQ[i].rob_src2;
                    FU[j].sl_no    = IQ[i].sl_no;
                    FU[j].values = IQ[i].values;

                    if(FU[j].op_type == 1){
                        FU[j].no_of_cycles = 2;
                    }
                    else if(FU[j].op_type == 2){
                        FU[j].no_of_cycles = 5;
                    }
                    else if(FU[j].op_type == 0){
                        FU[j].no_of_cycles = 1;
                    }
                return j;
            }
        }  
    }

    void execute(){
        int k = 0; 
        for(int i = 0; i < width*5 ;i++){
            if(FU[i].no_of_cycles == 1){
                wakeup(i);
                for(int j = 0;j < 67;j++){
                    if(FU[i].dst == RMT_TABLE[j].rob_tag){
                        RMT_TABLE[j] = {-1,0};
                    }

                }
                FU[i].values.WB_AC = advance_cycle + 1;
                k = add_to_wb(i,k);
                FU[i] = {-1,-1,-1,-1,0,0,0,-1,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
                FU_COUNT--;
            }
            else if(FU[i].no_of_cycles > 1){
                FU[i].no_of_cycles--;
                FU[i].values.EX_NC++;
            }
        }
    }
    void wakeup(int i){
        for(int j = 0; j < width;j++){
                if(RR[j].rs1 == FU[i].dst){
                    RR[j].rob_src1 = 0;   
                }

                if(DI[j].rs1 == FU[i].dst){
                    DI[j].rob_src1 = 0;   
                }

                if(RR[j].rs2 == FU[i].dst){
                    RR[j].rob_src2 = 0;   
                }

                if(DI[j].rs2 == FU[i].dst){
                    DI[j].rob_src2 = 0;   
                }

        }
        for(int k = 0;k < iq_size; k++){
            if(IQ[k].rs2_tag == FU[i].dst){
                    IQ[k].rob_src2 = 0;
                    IQ[k].rs2_rdy = 1;   
                }
            if(IQ[k].rs1_tag == FU[i].dst){
                    IQ[k].rob_src1 = 0;
                    IQ[k].rs1_rdy = 1;   
                }
            }
        }

    int add_to_wb(int i,int k){
        if(k == width*5){
            k = 0;
        }
        for(int j = k ; j < width*5 ; j++){
            if(WB[j].empty == false){
                    FU[i].valid = false;
                    WB[j].op_type = FU[i].op_type;
                    WB[j].empty = true;
                    WB[j].dst = FU[i].dst;
                    WB[j].rs1 = FU[i].rs1;
                    WB[j].rs2 = FU[i].rs2;
                    WB[j].rob_src1 = FU[i].rob_src1;
                    WB[j].rob_src2 = FU[i].rob_src2;
                    WB[j].sl_no    = FU[i].sl_no;
                    WB[j].values = FU[i].values;
                return j;
            }
        }
        return 0;  
    }

    void writeback(){
        for(int i = 0; i < width*5 ;i++){
            if(WB[i].empty == true){
                WB[i].values.RT_AC = advance_cycle + 1;
                ROB_TABLE[WB[i].dst].rdy = 1;
                ROB_TABLE[WB[i].dst].values = WB[i];
                WB[i] = {-1, -1,-1,-1,0,0,0,-1,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};  
            }                
            }
        }

    void retire(){
        int remove_flag =0;
        int head_temp = head;
        int total_empty = 0;
        for(int i = 0; i < rob_size ; i++){
            if(ROB_TABLE[i].rdy == 1){
                ROB_TABLE[i].values.values.RT_NC++;
            }}
        for(int i = head_temp; i < head_temp + width ; i++){
            if(ROB_TABLE[i].rdy == 1){
                remove_flag++;
            }
        }
        if(remove_flag == width){
                for(int i = head_temp; i < head_temp + width ; i++){
                    //RMT_TABLE[ROB_TABLE[i].dst] = {-1 , 0};
                    printROBArray(ROB_TABLE[i]);
                    ROB_TABLE[head].dst = -1;
                    ROB_TABLE[head].pc  = -1;
                    ROB_TABLE[head].rdy = 0;
                    head++;
                    if(head == rob_size){
                        head = 0;
                    }
                }
        }
        else{
            for(int i = 0; i < rob_size ; i++){
            if(ROB_TABLE[i].dst == -1){
                total_empty++;
            }
            if(total_empty == rob_size){
                end_of_stage = 1;
            }
            else{
                end_of_stage = 0;
            }
        }
        }
    }



    int findSmallest() {
    int min = IQ[0].seq; 
    for (int i = 1; i < iq_size; i++) {
        if (IQ[i].seq < min && IQ[i].seq != 0) {
            min = IQ[i].seq; 
        }
    }
    return min;
    }





    void print(pipelining* array){
        printf("===== contents =====");
        for (int i = 0; i < width; ++i) {
           printf("\n");
                printf("  %d",array[i].op_type);
                printf("  %d",array[i].dst);
                printf("  %d",array[i].rs1);
                printf("  %d",array[i].rs2);
                printf("  %d",array[i].empty);
                printf("  %d",array[i].rob_src1);
                printf("  %d",array[i].rob_src2);
                printf("\n");
        }
    }


    void print_iq(){
        printf("===== contents =====");
        for (int i = 0; i < iq_size; ++i) {
           printf("\n");
                printf("  %d",IQ[i].valid);
                printf("  %d",IQ[i].dst_tag);
                printf("  %d",IQ[i].rs1_rdy);
                printf("  %d",IQ[i].rs1_tag);
                printf("  %d",IQ[i].rs2_rdy);
                printf("  %d",IQ[i].rs2_tag);
                printf("  %d",IQ[i].seq);
                printf("  %d",IQ[i].rob_src1);
                printf("  %d",IQ[i].rob_src2);
                printf("\n");
        }
    }

        void print_ex(){
        printf("===== contents =====");
        for (int i = 0; i < width*5 ; ++i) {
           printf("\n");

                printf("  %d",FU[i].valid);
                printf("  %d",FU[i].dst);
                printf("  %d",FU[i].rs1);
                printf("  %d",FU[i].rs2);
                printf("  %d",FU[i].rob_src1);
                printf("  %d",FU[i].rob_src2);
                printf("  %d",FU[i].no_of_cycles);

                printf("\n");
                
        }
    }

        void print_WB(){
        printf("===== contents =====");
        for (int i = 0; i < width*5 ; ++i) {
           printf("\n");
                printf("  %d", WB[i].empty);
                printf("  %d", WB[i].dst);
                printf("  %d", WB[i].rs1);
                printf("  %d", WB[i].rs2);
                printf("  %d", WB[i].rob_src1);
                printf("  %d", WB[i].rob_src2);
                printf("\n");
                
        }
    }
        void print_ROB(){
        printf("===== contents =====");
        for (int i = 0; i < rob_size ; ++i) {
           printf("\n");
                printf("  %d", ROB_TABLE[i].dst);
                printf("  %d", ROB_TABLE[i].rdy);
                printf("  %d", ROB_TABLE[i].pc);
                printf("\n");
                
        }
    }
        void print_RMT(){
        printf("===== contents =====");
        for (int i = 0; i < 67 ; ++i) {
           printf("\n");
                printf("  %d", RMT_TABLE[i].valid);
                printf("  %d", RMT_TABLE[i].rob_tag);
                printf("\n");
                
        }
    }


    void switch_array(pipelining*& array1, pipelining*& array2){
        pipelining* temp = array1;
        //array2 = temp;
        for(int i = 0 ;i<width;i++){
            array2[i] = array1[i];
        }
    }

    void printROBArray(ROB &rob) {
    printf("\n%d fu{%d} src{%d,%d} dst{%d} ", 
        rob.values.sl_no,                // Program counter
        rob.values.op_type,    // Functional unit
        rob.rs1,        // Source 1
        rob.rs2,        // Source 2
        rob.dst         // Destination
    );
       rob.values.values.FE_NC = rob.values.values.DE_AC - rob.values.values.FE_AC;
        rob.values.values.DE_NC = rob.values.values.RN_AC - rob.values.values.DE_AC;
        rob.values.values.RN_NC = rob.values.values.RR_AC - rob.values.values.RN_AC;
        rob.values.values.RR_NC = rob.values.values.DI_AC - rob.values.values.RR_AC;
        rob.values.values.DI_NC = rob.values.values.IS_AC - rob.values.values.DI_AC;
        rob.values.values.WB_NC = rob.values.values.RT_AC - rob.values.values.WB_AC;

    // Print pipeline stage counters
    printf("FE{%d,%d} ", rob.values.values.FE_AC, rob.values.values.FE_NC);
    printf("DE{%d,%d} ", rob.values.values.DE_AC, rob.values.values.DE_NC);
    printf("RN{%d,%d} ", rob.values.values.RN_AC, rob.values.values.RN_NC);
    printf("RR{%d,%d} ", rob.values.values.RR_AC, rob.values.values.RR_NC);
    printf("DI{%d,%d} ", rob.values.values.DI_AC, rob.values.values.DI_NC);
    printf("IS{%d,%d} ", rob.values.values.IS_AC, rob.values.values.IS_NC);
    printf("EX{%d,%d} ", rob.values.values.EX_AC, rob.values.values.EX_NC);
    printf("WB{%d,%d} ", rob.values.values.WB_AC, rob.values.values.WB_NC);
    printf("RT{%d,%d}\n", rob.values.values.RT_AC, rob.values.values.RT_NC);
}
};

int main (int argc, char* argv[])
{
    //FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    uint64_t pc; // Variable holds the pc read from input file

    argv[0] = strdup("D:\\vscode\\programs\\project3_read_trace\\cpp_files\\sim_proc.cc");
    argv[1] = strdup("16");
    argv[2] = strdup("8");
    argv[3] = strdup("1");
    argv[4] = strdup("D:\\vscode\\programs\\project3_read_trace\\cpp_files\\val_trace_gcc1");
    argc = 5;
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // while(fscanf(FP, "%llx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //     printf("%llx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    out_of_order ooo(params.width, params.rob_size, params.iq_size);

    do{
        if(ooo.advance_cycle == 84){
            printf("nkjnhj");
        }
        ooo.retire();
        ooo.writeback();
        ooo.execute();
        ooo.issue();
        ooo.dispatch();
        ooo.regread();
        ooo.rename();
        ooo.decode();
        ooo.fetch();
        printf("DE");
        ooo.print(ooo.DE);
        printf("RN");
        ooo.print(ooo.RN);
        printf("RR");
        ooo.print(ooo.RR);
        printf("DI");
        ooo.print(ooo.DI);
        printf("IQ");
        ooo.print_iq();
        printf("ex");
        ooo.print_ex();
        printf("wb");
        ooo.print_WB();
        printf("hi");
        ooo.print_ROB();
        printf("rmt");
        ooo.print_RMT();
        printf("\n%d",ooo.advance_cycle);
        }
    //while(!(ooo.end_of_file && ooo.end_of_stage));
    while(ooo.advance_cycle < 100);
    return 0;
}
