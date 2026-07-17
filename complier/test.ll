// ============================================================
// 1. Recursive factorial
// ============================================================
fact(yoho n) {
    yedi (n <= 1) {
        firta 1;
    }
    firta n * fact(n - 1);
}

// ============================================================
// 2. Iterative Fibonacci (returns the nth Fibonacci number)
// ============================================================
fib(yoho n) {
    yoho a = 0;
    yoho b = 1;
    yoho i = 2;
    yoho temp;

    yedi (n == 0) {
        firta 0;
    }
    yedi (n == 1) {
        firta 1;
    }

    jabsamma (i <= n) {
        temp = a + b;
        a = b;
        b = temp;
        i = i + 1;
    }
    firta b;
}

// ============================================================
// 3. Check if a number is prime (returns 1 if prime, 0 otherwise)
// ============================================================
isPrime(yoho num) {
    yoho i = 2;
    yedi (num < 2) {
        firta 0;
    }
    jabsamma (i * i <= num) {
        yedi (num % i == 0) {
            firta 0;
        }
        i = i + 1;
    }
    firta 1;
}

// ============================================================
// 4. Print all primes up to N using a for loop
// ============================================================
printPrimes(yoho N) {
    ferini (yoho i = 2; i <= N; i = i + 1) {
        yedi (isPrime(i) == 1) {
            bhan(i);
        }
    }
}

// ============================================================
// 5. Sum of digits of a number (using do‑while)
// ============================================================
sumDigits(yoho num) {
    yoho sum = 0;
    yoho n = num;
    yedi (n < 0) {
        n = -n;  // handle negative numbers
    }
    gar {
        sum = sum + (n % 10);
        n = n / 10;
    } jabsamma (n > 0);
    firta sum;
}

// ============================================================
// 6. Main program
// ============================================================
yoho number;
yoho choice;

bhan("=== Advanced Demo ===");
bhan("Enter a positive integer: ");
sun("Number: ", number);

// ---- Factorial ----
bhan("Factorial of ", number, " = ", fact(number));

// ---- Prime check ----
yedi (isPrime(number) == 1) {
    bhan(number, " is prime.");
} natra {
    bhan(number, " is not prime.");
}

// ---- Fibonacci ----
yoho fibVal = fib(number);
bhan("Fibonacci(", number, ") = ", fibVal);

// ---- Sum of digits ----
yoho digitSum = sumDigits(number);
bhan("Sum of digits = ", digitSum);

// ---- Print all primes up to the number ----
bhan("Primes up to ", number, ":");
printPrimes(number);

// ---- Demonstrate break and continue ----
bhan("Demonstrating break and continue:");
ferini (yoho i = 1; i <= 10; i = i + 1) {
    yedi (i == 5) {
        bhan("  Breaking at i=5");
        voyo;                // break
    }
    yedi (i % 2 == 0) {
        aghibad;             // skip even numbers
    }
    bhan("  i = ", i);
}

// ---- Switch example ----
bhan("Switch example:");
yochai (number % 3) {
    yo 0 : {
        bhan("Number is divisible by 3");
    }
    yo 1 : {
        bhan("Number mod 3 is 1");
    }
    abayeiho : {
        bhan("Number mod 3 is 2");
    }
}

// ---- Input multiple numbers ----
yoho a, b, c;
sun("Enter three numbers: ", a, b, c);
bhan("You entered: ", a, ", ", b, ", ", c);