//
// Created by farenain on 15/3/26.
//
int main() {
    volatile int i;
    // Empty inifinite loops are technically
    // undefined behavior, so avoid it
    while (true) i = 42;
}