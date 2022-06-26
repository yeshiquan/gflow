#include <iostream>
#include <vector>

enum byte_code {
    op_neg,  //  negate the top stack entry
    op_add,  //  add top two stack entries
    op_sub,  //  subtract top two stack entries
    op_mul,  //  multiply top two stack entries
    op_div,  //  divide top two stack entries
    op_int,  //  push constant integer into the stack
};

class vmachine {
   public:
    vmachine(unsigned stackSize = 4096)
        : stack(stackSize), stack_ptr(stack.begin()) {}

    int top() const { return stack_ptr[-1]; };
    int pop() {
        stack_ptr--;
        return *stack_ptr;
    }
    int *ptop() { return &stack_ptr[-1]; }
    void push(int v) {
        stack_ptr[0] = v;
        stack_ptr++;
    }
    void execute(std::vector<int> const &code);

   private:
    std::vector<int> stack;
    std::vector<int>::iterator stack_ptr;
};

void vmachine::execute(std::vector<int> const &code) {
    std::vector<int>::const_iterator pc = code.begin();
    stack_ptr = stack.begin();
    int v = 0;

    while (pc != code.end()) {
        switch (*pc++) {
            case op_neg:
                *ptop() = -top();
                break;

            case op_add:
                v = pop();
                *ptop() += v;
                break;

            case op_sub:
                v = pop();
                *ptop() -= v;
                break;

            case op_mul:
                v = pop();
                *ptop() *= v;
                break;

            case op_div:
                v = pop();
                *ptop() /= v;
                break;

            case op_int:
                push(*pc);
                pc++;
                break;
        }
    }
}

int main() {
    std::vector<int> code;
    code.push_back(op_int);
    code.push_back(2);
    code.push_back(op_int);
    code.push_back(3);
    code.push_back(op_add);
    code.push_back(op_int);
    code.push_back(4);
    code.push_back(op_mul);
    vmachine mach;
    mach.execute(code);
    int result = mach.top();
    std::cout << "result -> " << result << std::endl;
}