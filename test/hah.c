#include <stdio.h>

int main(){
        int a = 123;
        int b = 0;
        if (a%2 == 0) {
                int a = 456;
                b=a;
        }
        printf("%d", b);
        return 0;
}


