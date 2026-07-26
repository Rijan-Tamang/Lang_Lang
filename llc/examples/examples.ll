// programs.ll
// A single, self-contained, ready-to-run program that exercises every
// rule in the language grammar: decleration, block, functiondec,
// functiondef, paramlist/param, returnStatement, printStatement,
// inputStatement, ifStatement/tesovaye/natra, switchStatement/caseClause/
// defaultClause, whileStatement, dowhileStatement, forStatement,
// breakstatement, continueStatement, emptyStatement, exprStatement,
// and the full expression hierarchy (assignment, logicalOr, logicalAnd,
// equality, relational, additive, multiplicative, unary, call, arguments,
// primary).

// forward declaration (functiondec)
multiply(yoho a, yoho b);

// function definitions (functiondef, paramlist, block, returnStatement)
add(yoho a, yoho b) {
    firta a + b;
}

multiply(yoho a, yoho b) {
    firta a * b;
}

isPositive(yoho n) {
    yedi (n > 0) {
        firta 1;
    } natra {
        firta 0;
    }
}
// decleration with multiple identifiers and initializers
yoho x = 10, y = 3, total, choice;

// exprStatement: call
bhan("Starting program");

// exprStatement: assignment, plus call as expression
total = add(x, y);
bhan("x + y =", total);
bhan("x * y =", multiply(x, y));

// logicalOr / logicalAnd / equality / relational / additive / multiplicative / unary
yedi (x > 0 && y > 0) {
    bhan("both positive");
} tesovaye (x == 0 || y == 0) {
    bhan("one is zero");
} natra {
    bhan("something is negative");
}

bhan(-x, +y, !isPositive(x));
bhan((x + y) * 2 - 1 % 3);

// inputStatement
bhan("Pick 1, 2, or 3:");
sun("choice: ", choice);

// switchStatement, caseClause, defaultClause, break
yochai (choice) {
    yo 1:
        bhan("You picked one");
        //vayo;
    yo 2:
        bhan("You picked two");
        //vayo;
    abayeiho:
        bhan("Unknown choice");
}

// whileStatement, break, continue
yoho i = 0;
jabasamma (i < 10) {
    i = i + 1;
    yedi (i % 2 == 0) {
        aghibadh;
    }
    yedi (i > 7) {
        vayo;
    }
    bhan("odd i =", i);
}

// dowhileStatement
yoho j = 0;
gar {
    bhan("j =", j);
    j = j + 1;
} jabasamma (j < 3);

// forStatement
ferini (yoho k = 0; k < 5; k = k + 1) {
    bhan("k =", k);
}

// emptyStatement
;