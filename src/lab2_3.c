#include <stdio.h>

int is_prime(int n) {
   if (n < 2) {
       return 0;
   }
    for (int i = 2; i <= n / i; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    int n;

    printf("Enter an integer n (>= 2): ");
  if (scanf("%d", &n) != 1) {
      printf("Error");
      return 1;
}

   if (n < 2) {
       printf("Error");
   } else {
       printf("Prime numbers: ");
       for (int i = 2; i <= n; i++) {
           if (is_prime(i)) {
               printf("%d ", i);
           }
       }
       printf("\n");
   }

    return 0;
}
