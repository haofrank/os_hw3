class Instruction
{
public:
    char command;
    int page;
    Instruction(char cmd, int page);
    void get_next_instruction(int &operation, int &vpage);
};

Instruction::Instruction(char cmd, int page)
{
    this->command=cmd;
    this->page = page;
}

