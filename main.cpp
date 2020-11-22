#include <iostream>
#include <queue>
#include <set>
#include <vector>
#include <bits/stdc++.h>


using namespace std;

enum OPS {
    ADD, SUB, ADD_f, SUB_f, MUL_f, DIV_f, LOAD, STORE,
};
vector<string> ori_ins;
vector<int> issue_counter(200);
vector<int> execute_counter(200);
vector<int> memory_counter(200);
vector<int> writeback_counter(200);
vector<int> commit_counter(200);

const char * DELIM2 = ", \t()";
#define ADD_CYCLE 1
#define ADD_F_CYCLE 3
#define MULT_F_CYCLE 20
#define LS_CYCLE 1

typedef struct _insturction {
    OPS op; // operation
    int rd; // destination
    int rs; // source
    bool point_rob_rs;
    int rt; // target
    bool point_rob_rt;
    int period; // preiod
    _insturction(OPS op, int rd, int rs, int rt)
            : op(op), rs(rs), rd(rd), rt(rt) {
        switch (op) {
            case ADD:
            case SUB:
                this->period = 1;
                break;
            case ADD_f:
            case SUB_f:
                this->period = 3;
                break;
            case MUL_f:
            case DIV_f:
                this->period = 20;
                break;
            case LOAD:
            case STORE:
                this->period = 1;
                break;
        }
    };
    _insturction() {};

} instruction;

typedef struct _ROB_element {
    instruction ins;
    OPS op;
    int rob_id;
    int value;
    bool done; // excute done
    bool write_back_done; // write back done
    bool committed;// commit done
    int period_left;
    bool require_memory;

    _ROB_element(instruction ins, OPS op, int rob_id)
            : ins(ins), op(op), rob_id(rob_id) {
        this->period_left = ins.period;
        this->done = false;
        this->write_back_done = false;
        this->committed = false;
        this->require_memory = false;
    };

} ROB_element;
//struct ARF_element {
//};

typedef struct _RAT_element {
    bool point_rob;
    int refer_id;
    int value;

    _RAT_element(bool point_rob, int refer_id, int value)
            : point_rob(point_rob), refer_id(refer_id), value(value) {};
} RAT_element;

typedef struct _RAT_f_element {
    bool point_rob;
    int refer_id;
    int value;

    _RAT_f_element(bool point_rob, int refer_id, int value)
            : point_rob(point_rob), refer_id(refer_id), value(value) {};
} RAT_f_element;


vector<ROB_element> ROB;
vector<RAT_element> RAT;
vector<RAT_f_element> RAT_f;
queue<instruction> inss;

int RS_add_size = 0;
int RS_mul_size = 0;
int RS_add_f_size = 0;
int RS_mul_f_size = 0;
int RS_ls_size = 0;

#define RS_ADD_SIZE 2
#define RS_ADD_F_SIZE 3
#define RS_MUL_F_SIZE 2
#define RS_LS_SIZE 3

void issue();
void excute();
void memory();
void write_back();
void commit();

static int counter = 1;

void getinstruction(ifstream &infile, instruction ins1);
void print(ofstream & ofile);

int main() {
    ifstream infile;
    infile.open("../input.txt",ios::in);
    if (infile.fail()) {
        cout << "Can't open file: input.txt" << endl;
        exit(-1);
    }
    ofstream outfile;
    outfile.open("../output.txt",ios::out);
    instruction ins1;
    getinstruction(infile,ins1);

    for (int i = 0; i < 32; i++) {
        RAT.push_back(RAT_element(false, 0, 0));
        RAT_f.push_back(RAT_f_element(false, 0, 0));
    }
    RAT[1].value = 10;
    RAT[2].value = 20;

//    while (inss.size() > 0) {
    int size = inss.size();
    while (commit_counter[size - 1] == 0) {
//        cout << counter << endl;
        commit();
        write_back();
        memory();
        excute();
        issue();
        counter++;
    }
    print(outfile);
    outfile.close();
    //for (int i = 0; i < size; i++) {
//        cout << issue_counter[i] << " ";
//        cout << execute_counter[i] << " ";
//        if(memory_counter[i] == 0)
//            cout << "- ";
//        else
//            cout << memory_counter[i] << " ";
//        cout << writeback_counter[i] << " ";
//        cout << commit_counter[i] << " ";
//        cout << endl;
//    }
//    cout << endl;
//    for (int i = 0; i < 32; i++) {
//        cout << RAT[i].value << " ";
//    }
//    cout << endl;
//    for (int i = 0; i < 32; i++) {
//        cout << RAT_f[i].value << " ";
//    }
//    cout << endl;
}


void issue() {
    // if reservation station not full
    if (inss.size() <= 0)
        return;

    instruction ins = inss.front();
    switch (ins.op) {
        case OPS::ADD:
        case OPS::SUB:
            if (RS_add_size < RS_ADD_SIZE) {
                // renaming
                if (RAT[ins.rs].point_rob == true) {
                    ins.point_rob_rs = true;
                    ins.rs = RAT[ins.rs].refer_id;
                } else {
                    ins.point_rob_rs = false;
                    ins.rs = RAT[ins.rs].value;
                }
                if (RAT[ins.rt].point_rob == true) {
                    ins.point_rob_rt = true;
                    ins.rt = RAT[ins.rt].refer_id;
                } else {
                    ins.rt = RAT[ins.rt].value;
                }
                RAT[ins.rd].refer_id = ROB.size();
                RAT[ins.rd].point_rob = true;

                // push to ROB buffer and RS_add vector
                RS_add_size++;
                ROB.push_back(ROB_element(ins, ins.op, ROB.size()));
                inss.pop();
//                cout << "issue a command" << endl;
                issue_counter[ROB.size() - 1] = counter;
            }
            break;
        case OPS::ADD_f:
        case OPS::SUB_f:
            if (RS_add_f_size < RS_ADD_F_SIZE) {
                if (RAT_f[ins.rs].point_rob == true) {
                    ins.point_rob_rs = true;
                    ins.rs = RAT_f[ins.rs].refer_id;
                } else {
                    ins.point_rob_rs = false;
                    ins.rs = RAT_f[ins.rs].value;
                }
                if (RAT_f[ins.rt].point_rob == true) {
                    ins.point_rob_rt = true;
                    ins.rt = RAT_f[ins.rt].refer_id;
                } else {
                    ins.rt = RAT_f[ins.rt].value;
                }
                RAT_f[ins.rd].refer_id = ROB.size();
                RAT_f[ins.rd].point_rob = true;

                // push to ROB buffer and RS_add vector
                RS_add_f_size++;
                ROB.push_back(ROB_element(ins, ins.op, ROB.size()));
                inss.pop();
//                cout << "issue a command" << endl;
                issue_counter[ROB.size() - 1] = counter;
            }
            break;
        case OPS::MUL_f:
        case OPS::DIV_f:
            if (RS_mul_f_size < RS_MUL_F_SIZE) {
                // renaming
                if (RAT_f[ins.rs].point_rob == true) {
                    ins.point_rob_rs = true;
                    ins.rs = RAT_f[ins.rs].refer_id;
                } else {
                    ins.point_rob_rs = false;
                    ins.rs = RAT_f[ins.rs].value;
                }
                if (RAT_f[ins.rt].point_rob == true) {
                    ins.point_rob_rt = true;
                    ins.rt = RAT_f[ins.rt].refer_id;
                } else {
                    ins.rt = RAT_f[ins.rt].value;
                }
                RAT_f[ins.rd].refer_id = ROB.size();
                RAT_f[ins.rd].point_rob = true;

                // push to ROB buffer and RS_add vector
                RS_mul_f_size++;
                ROB.push_back(ROB_element(ins, ins.op, ROB.size()));
                inss.pop();
//                cout << "issue a command" << endl;
                issue_counter[ROB.size() - 1] = counter;
            }
            break;
        case OPS::STORE:
        case OPS::LOAD:
            if (RS_ls_size < RS_LS_SIZE) {
                if (RAT[ins.rs].point_rob == true) {
                    ins.point_rob_rs = true;
                } else {
                    ins.point_rob_rs = false;
                }

                RAT_f[ins.rd].refer_id = ROB.size();
                RAT_f[ins.rd].point_rob = true;
                RS_ls_size ++;
                ROB.push_back(ROB_element(ins, ins.op, ROB.size()));
                ROB[ROB.size() - 1].require_memory = true;
                inss.pop();
                //cout << "issue a command" << endl;
                issue_counter[ROB.size() - 1] = counter;
            }
            break;
        default:
            cout << "something wrong" << endl;
    }
}

void excute() {
    for (auto i = ROB.begin(); i != ROB.end(); ++i) {
        if (i->done) continue;
        if (i->ins.point_rob_rs == true) {
            if (ROB[i->ins.rs].done) {
                i->ins.rs = ROB[i->ins.rs].value;
                i->ins.point_rob_rs = false;
            }
        }
        if (i->ins.point_rob_rt == true) {
            if (ROB[i->ins.rt].done) {
                i->ins.rt = ROB[i->ins.rt].value;
                i->ins.point_rob_rt = false;
            }
        }
        if (i->ins.point_rob_rs == true || i->ins.point_rob_rt == true) {
            continue;
        } else {
            i->period_left--;
        }
        // tyring to excute the command
//        i->period_left--;
        if (i->period_left <= 0) {
            i->done = true;
            switch (i->op) {
                case OPS::ADD:
                    i->value = i->ins.rs + i->ins.rt;
                    RS_add_size--;
                    break;
                case OPS::SUB:
                    i->value = i->ins.rs - i->ins.rt;
                    RS_add_size--;
                    break;
                case OPS::ADD_f:
                    i->value = i->ins.rs + i->ins.rt;
                    RS_add_f_size--;
                    break;
                case OPS::SUB_f:
                    i->value = i->ins.rs - i->ins.rt;
                    RS_add_f_size--;
                    break;
                case OPS::MUL_f:
                    i->value = i->ins.rs * i->ins.rt;
                    RS_mul_f_size--;
                    break;
                case OPS::DIV_f:
                    i->value = i->ins.rs / i->ins.rt;
                    RS_mul_f_size--;
                    break;
                case OPS::LOAD:
                case OPS::STORE:
                    i->value = i->ins.rs;
                    RS_ls_size--;
                    break;
                default:
                    cout << "something wrong in function execute!" << endl;

            }
            execute_counter[i->rob_id] = counter;
        }
    }
};

void memory() {
    for (auto i = ROB.begin(); i != ROB.end(); ++i) {
        if (i->done && i->require_memory) {
            i->require_memory = false;
            memory_counter[i->rob_id] = counter;
            break;
        }
    }
};

void write_back() {
    for (auto i = ROB.begin(); i != ROB.end(); ++i) {
        if (i->done && i->write_back_done == false && i->require_memory == false) {
            i->write_back_done = true;
            if (i->op == ADD || i->op == SUB || i->op == LOAD || i-> op == STORE)
                RAT[i->ins.rd].value = i->value;
            else
                RAT_f[i->ins.rd].value = i->value;
            writeback_counter[i->rob_id] = counter;
            break;
        }
    }
};

void commit() {
    for (auto i = ROB.begin(); i != ROB.end(); ++i) {
        if (i->write_back_done && i->committed)
            continue;
        if (i->write_back_done && i->committed == false) {
            i->committed = true;
            commit_counter[i->rob_id] = counter;
            break;
        } else
            break;
    }
};

void getinstruction(ifstream &infile, instruction ins1)
{
    char buf[30];
    string buf1;

    while (infile.getline(buf,65535)) {
        buf1=buf;
        ori_ins.push_back(buf1);
        char * word;
        vector<char *> words;
        string op;
        string token, label, dest, src, trgt;
        int i_src=0,i_trgt=0,i_dest=0,i_label=0;
        word = strtok(buf, DELIM2);//DELIM2=", \t()";
        if(word == NULL)continue;
        while (word) {
            words.push_back(word);
            word = strtok(NULL, DELIM2);
        }

        // Set the operator:
        op = words[0];
        // Parse the op:


        if (op.compare("Sd")==0){
            dest = words[1];
            src =words[2];
            trgt = words[3];
            ins1.op=OPS::STORE;
            ins1.period = LS_CYCLE;
        }else if(op.compare("Ld")==0) {
            dest = words[1];
            src = words[2];
            trgt = words[3];
            ins1.op=OPS::LOAD;
            ins1.period = LS_CYCLE;
        } else if (op.compare("Add")==0) {
            dest = words[1];
            src = words[2];
            trgt = words[3];
            ins1.op=OPS::ADD;
            ins1.period = ADD_CYCLE;
        } else if (op.compare("Sub")==0) {
            dest = words[1];
            src = words[2];
            trgt = words[3];
            ins1.op=OPS::SUB;
            ins1.period = ADD_CYCLE;
        } else if (op.compare("Sub.d")==0) {
            dest = words[1];
            src = words[2];
            trgt = words[3];
            ins1.op=OPS::SUB_f;
            ins1.period = ADD_F_CYCLE;
        } else if (op.compare("Add.d")==0){
            dest = words[1];
            src = words[2];
            trgt = words[3];
            ins1.op=OPS::ADD_f;
            ins1.period = ADD_F_CYCLE;
        }else if (op.compare("Mul.d")==0){
            dest = words[1];
            src =words[2];
            trgt = words[3];
            ins1.op=OPS::MUL_f;
            ins1.period = MULT_F_CYCLE;
        }
        else{
            cout<<"Not a valid operation code:"<<op<<endl;
            continue;
        }
        ins1.rd = atoi(&dest.c_str()[1]);
        ins1.rs = atoi(&src.c_str()[1]);
        ins1.rt = atoi(&trgt.c_str()[1]);
        inss.push(ins1);
    }
}

void print(ofstream & ofile)
{
    cout<<"Completed. Please check \"output.txt\"."<<endl;
    ofile<<"			"<<"ISSUE"<<"	"<<"EX"<<"	"<<"MEM"<<"	"<<"WB"<<"	"<<"COMMIT"<<endl;
    for(int j=0;j< ori_ins.size();j++){
        ofile<< ori_ins[j]<<"	"<<issue_counter[j]<<"	"<<execute_counter[j]<<"	";
        if(memory_counter[j]!=0) ofile<<memory_counter[j]<<"	";
        else ofile<<"	";
        ofile<<writeback_counter[j]<<"	"<<commit_counter[j]<<endl;
    }
    ofile<<endl;
    ofile<<"Register Values:"<<endl;

    for(int j=0;j<32;j++){
        ofile<<"R"<<j<<"	";
    }
    ofile << endl;
    for(int j=0;j<32;j++){
        ofile<<RAT[j].value<<"	";
    }
    ofile<<endl;
    for(int j=0;j<32;j++){
        ofile<<"F"<<j<<"	";
    }
    ofile << endl;
    for(int j=0;j<32;j++){
        ofile<<RAT_f[j].value<<"	";
    }
    ofile<<endl;
}