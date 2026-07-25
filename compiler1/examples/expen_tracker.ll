// KU STUDENT WEEKLY BUDGET & EXPENSE TRACKER
// Written against the provided grammar.
// NOTE: comment syntax ("//") is assumed, since the grammar excerpt does not
// define one. Strip these lines if the lexer does not support comments.
//
// Grammar has no array/map type, so up to MAX expenses per week are stored
// in individually-named slot variables (amount0..amount7 / cat0..cat7) and
// "indexed" using a for-loop whose counter is dispatched through a switch
// (yochai) statement. See analysis notes for details.

// ---- prototype demo (functiondec), matched by a functiondef below ----
categoryName(yoho code);

// ---------------- global state ----------------
yoho MAX = 8;
yoho count = 0;
yoho totalSpent = 0;
yoho weeklyBudget = 0;

yoho amount0 = 0, amount1 = 0, amount2 = 0, amount3 = 0,
     amount4 = 0, amount5 = 0, amount6 = 0, amount7 = 0;
yoho cat0 = 0, cat1 = 0, cat2 = 0, cat3 = 0,
     cat4 = 0, cat5 = 0, cat6 = 0, cat7 = 0;

yoho foodBudget = 0, transportBudget = 0, rentBudget = 0, booksBudget = 0,
     entBudget = 0, mobileBudget = 0, otherBudget = 0;
yoho foodSpent = 0, transportSpent = 0, rentSpent = 0, booksSpent = 0,
     entSpent = 0, mobileSpent = 0, otherSpent = 0;

yoho running = 1;
yoho choice = 0;

// ---------------- helpers ----------------

categoryName(yoho code) {
    yochai (code) {
        yo 1: firta "Food";
        yo 2: firta "Transport";
        yo 3: firta "Rent/Hostel";
        yo 4: firta "Books/Stationery";
        yo 5: firta "Entertainment";
        yo 6: firta "Mobile/Data";
        abayeiho: firta "Other";
    }
}

printMenu() {
    bhan("========================================");
    bhan("   KU STUDENT WEEKLY BUDGET TRACKER");
    bhan("========================================");
    bhan("1. Set weekly budget");
    bhan("2. Add an expense");
    bhan("3. View all expenses");
    bhan("4. View budget status");
    bhan("5. Exit");
}

setBudget() {
    sun("Enter overall weekly budget: ", weeklyBudget);
    sun("Food budget: ", foodBudget);
    sun("Transport budget: ", transportBudget);
    sun("Rent/Hostel budget: ", rentBudget);
    sun("Books/Stationery budget: ", booksBudget);
    sun("Entertainment budget: ", entBudget);
    sun("Mobile/Data budget: ", mobileBudget);
    sun("Other budget: ", otherBudget);
    bhan("Budget saved.");
}

addExpense() {
    yoho code = 0, amt = 0;

    yedi (count >= MAX) {
        bhan("Storage full! Cannot add more expenses this week.");
        firta;
    }

    bhan("Categories: 1 Food  2 Transport  3 Rent  4 Books  5 Entertainment  6 Mobile  7 Other");
    sun("Category code: ", code);
    sun("Amount spent: ", amt);

    yochai (count) {
        yo 0: amount0 = amt; cat0 = code;
        yo 1: amount1 = amt; cat1 = code;
        yo 2: amount2 = amt; cat2 = code;
        yo 3: amount3 = amt; cat3 = code;
        yo 4: amount4 = amt; cat4 = code;
        yo 5: amount5 = amt; cat5 = code;
        yo 6: amount6 = amt; cat6 = code;
        yo 7: amount7 = amt; cat7 = code;
    }

    count = count + 1;
    totalSpent = totalSpent + amt;

    yochai (code) {
        yo 1: foodSpent = foodSpent + amt;
        yo 2: transportSpent = transportSpent + amt;
        yo 3: rentSpent = rentSpent + amt;
        yo 4: booksSpent = booksSpent + amt;
        yo 5: entSpent = entSpent + amt;
        yo 6: mobileSpent = mobileSpent + amt;
        abayeiho: otherSpent = otherSpent + amt;
    }

    bhan("Expense recorded.");
}

viewExpenses() {
    yedi (count == 0) {
        bhan("No expenses recorded yet.");
        firta;
    }

    bhan("---- Expense List ----");
    ferini (yoho i = 0; i < count; i = i + 1) {
        yochai (i) {
            yo 0: bhan(i + 1, categoryName(cat0), amount0);
            yo 1: bhan(i + 1, categoryName(cat1), amount1);
            yo 2: bhan(i + 1, categoryName(cat2), amount2);
            yo 3: bhan(i + 1, categoryName(cat3), amount3);
            yo 4: bhan(i + 1, categoryName(cat4), amount4);
            yo 5: bhan(i + 1, categoryName(cat5), amount5);
            yo 6: bhan(i + 1, categoryName(cat6), amount6);
            yo 7: bhan(i + 1, categoryName(cat7), amount7);
        }
    }
    bhan("Total spent so far: ", totalSpent);
}

checkStatus() {
    bhan("---- Budget Status ----");
    bhan("Overall: spent", totalSpent, "of", weeklyBudget);

    yedi (totalSpent > weeklyBudget) {
        bhan("WARNING: You are OVER your weekly budget!");
    } natra {
        bhan("Within budget. Remaining:", weeklyBudget - totalSpent);
    }

    bhan("Food:", foodSpent, "/", foodBudget);
    yedi (foodSpent > foodBudget) { bhan("  -> over food budget"); }

    bhan("Transport:", transportSpent, "/", transportBudget);
    yedi (transportSpent > transportBudget) { bhan("  -> over transport budget"); }

    bhan("Rent:", rentSpent, "/", rentBudget);
    yedi (rentSpent > rentBudget) { bhan("  -> over rent budget"); }

    bhan("Books:", booksSpent, "/", booksBudget);
    yedi (booksSpent > booksBudget) { bhan("  -> over books budget"); }

    bhan("Entertainment:", entSpent, "/", entBudget);
    yedi (entSpent > entBudget) { bhan("  -> over entertainment budget"); }

    bhan("Mobile:", mobileSpent, "/", mobileBudget);
    yedi (mobileSpent > mobileBudget) { bhan("  -> over mobile budget"); }

    bhan("Other:", otherSpent, "/", otherBudget);
    yedi (otherSpent > otherBudget) { bhan("  -> over other budget"); }
}

// ---------------- driver ----------------

bhan("Welcome! Let's set up this week's budget first.");
setBudget();

jabasamma (running == 1) {
    printMenu();
    sun("Choose an option (1-5): ", choice);

    yedi (choice == 1) {
        setBudget();
    } tesovaye (choice == 2) {
        addExpense();
    } tesovaye (choice == 3) {
        viewExpenses();
    } tesovaye (choice == 4) {
        checkStatus();
    } tesovaye (choice == 5) {
        running = 0;
        bhan("Goodbye! Stay on budget.");
    } natra {
        bhan("Invalid option, try again.");
    }
}