#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <limits>

using namespace std;
using namespace chrono;

// ─────────────────────────────────────────────
//  Data structures
// ─────────────────────────────────────────────

struct Answer {
    string text;
    bool isCorrect;
};

struct Question {
    string text;
    vector<Answer> answers;      // up to 4 options
    int timeLimitSeconds;        // per-question time limit
    int points;                  // points awarded for correct answer
};

struct QuizResult {
    string questionText;
    string chosenAnswer;
    string correctAnswer;
    bool wasCorrect;
    bool timedOut;
    double timeTaken;            // seconds
};

struct LeaderboardEntry {
    string playerName;
    int score;
    int totalQuestions;
    double timeTaken;            // total seconds
};

// ─────────────────────────────────────────────
//  Utility helpers
// ─────────────────────────────────────────────

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printSeparator(char ch = '─', int len = 60) {
    cout << string(len, ch) << "\n";
}

void printCentered(const string& text, int width = 60) {
    int padding = max(0, (width - (int)text.size()) / 2);
    cout << string(padding, ' ') << text << "\n";
}

void printHeader(const string& title) {
    printSeparator('═');
    printCentered(title);
    printSeparator('═');
}

// Countdown timer displayed in the console
bool waitForInputWithTimer(int& choice, int timeLimitSeconds, int numOptions) {
    auto start = steady_clock::now();

    // Non-blocking input using a thread
    bool answered = false;
    bool timedOut = false;
    string inputStr;

    // We show a countdown — simplified blocking version with time check
    cout << "\n";
    while (true) {
        auto elapsed = duration_cast<seconds>(steady_clock::now() - start).count();
        int remaining = timeLimitSeconds - (int)elapsed;

        if (remaining <= 0) {
            timedOut = true;
            cout << "\n⏰  Time's up!\n";
            break;
        }

        // Show timer bar
        cout << "\r⏱  Time left: " << setw(2) << remaining << "s  [";
        int barWidth = 30;
        int filled = (remaining * barWidth) / timeLimitSeconds;
        string bar(filled, '█');
        bar += string(barWidth - filled, '░');
        cout << bar << "]  Your answer (1-" << numOptions << "): ";
        cout.flush();

        // Poll for input every 100ms
        // Use cin with a timeout trick via select (POSIX) or simple loop
        // For portability we do a simple blocking cin here after showing the timer
        // In a real app you'd use select() or threads; this is a console demo.

        // Since true non-blocking cin is platform-specific, we do one blocking read
        // after printing the timer once. Users see remaining time then type.
        if (cin >> inputStr) {
            try {
                choice = stoi(inputStr);
                if (choice >= 1 && choice <= numOptions) {
                    answered = true;
                    break;
                } else {
                    cout << "  ⚠  Enter a number between 1 and " << numOptions << "\n";
                }
            } catch (...) {
                cout << "  ⚠  Invalid input. Enter a number.\n";
            }
        }

        // Recalculate remaining after cin returned
        elapsed = duration_cast<seconds>(steady_clock::now() - start).count();
        if (timeLimitSeconds - (int)elapsed <= 0) {
            timedOut = true;
            cout << "\n⏰  Time's up!\n";
            break;
        }
    }

    return !timedOut;
}

// ─────────────────────────────────────────────
//  Quiz bank
// ─────────────────────────────────────────────

vector<Question> buildQuestionBank() {
    return {
        {
            "What does OOP stand for in C++?",
            {
                {"Object-Oriented Programming", true},
                {"Open Object Protocol",        false},
                {"Operator Overloading Process",false},
                {"Organised Output Parsing",    false}
            },
            15, 10
        },
        {
            "Which keyword is used to allocate memory dynamically in C++?",
            {
                {"malloc",  false},
                {"alloc",   false},
                {"new",     true},
                {"create",  false}
            },
            15, 10
        },
        {
            "What is the output of: cout << 5 / 2; (both integers)?",
            {
                {"2.5", false},
                {"2",   true},
                {"3",   false},
                {"0",   false}
            },
            20, 10
        },
        {
            "Which header file is required for cout/cin?",
            {
                {"<stdio.h>",  false},
                {"<conio.h>",  false},
                {"<iostream>", true},
                {"<string.h>", false}
            },
            15, 10
        },
        {
            "What is a pointer in C++?",
            {
                {"A variable that stores a value directly",        false},
                {"A variable that stores a memory address",        true},
                {"A function that returns a memory address",       false},
                {"A keyword used for dynamic memory allocation",   false}
            },
            20, 10
        },
        {
            "What does 'virtual' keyword enable in C++?",
            {
                {"Memory allocation",      false},
                {"Runtime polymorphism",   true},
                {"Compile-time binding",   false},
                {"Operator overloading",   false}
            },
            20, 10
        },
        {
            "Which of these is NOT a valid C++ loop?",
            {
                {"for",    false},
                {"while",  false},
                {"repeat", true},
                {"do",     false}
            },
            15, 10
        },
        {
            "What is the size of 'int' on most 64-bit systems?",
            {
                {"2 bytes", false},
                {"4 bytes", true},
                {"8 bytes", false},
                {"1 byte",  false}
            },
            15, 10
        },
        {
            "What does STL stand for?",
            {
                {"Standard Type Library",     false},
                {"Static Template Library",   false},
                {"Standard Template Library", true},
                {"Structured Text Language",  false}
            },
            15, 10
        },
        {
            "Which operator is used to access members through a pointer?",
            {
                {".",   false},
                {"->",  true},
                {"*",   false},
                {"&",   false}
            },
            15, 10
        }
    };
}

// ─────────────────────────────────────────────
//  Core quiz engine
// ─────────────────────────────────────────────

vector<QuizResult> runQuiz(const string& playerName, vector<Question>& questions, int numQ) {
    vector<QuizResult> results;
    int totalScore = 0;
    int maxScore   = 0;

    clearScreen();
    printHeader("  🎯 QUIZ START — Good Luck, " + playerName + "!  ");
    cout << "\n  Questions: " << numQ
         << "  |  Max score: " << numQ * 10 << " pts\n\n";
    this_thread::sleep_for(seconds(2));

    for (int i = 0; i < numQ && i < (int)questions.size(); ++i) {
        Question& q = questions[i];
        maxScore += q.points;

        clearScreen();
        printSeparator();
        cout << "  Question " << (i + 1) << " / " << numQ
             << "   |   +" << q.points << " pts"
             << "   |   ⏱ " << q.timeLimitSeconds << "s limit\n";
        printSeparator();
        cout << "\n  " << q.text << "\n\n";

        for (int j = 0; j < (int)q.answers.size(); ++j) {
            cout << "   [" << (j + 1) << "] " << q.answers[j].text << "\n";
        }

        auto qStart = steady_clock::now();
        int choice = 0;
        bool answered = waitForInputWithTimer(choice, q.timeLimitSeconds, (int)q.answers.size());
        double elapsed = duration_cast<milliseconds>(steady_clock::now() - qStart).count() / 1000.0;

        QuizResult res;
        res.questionText = q.text;
        res.timedOut     = !answered;
        res.timeTaken    = elapsed;

        // Find correct answer text
        for (auto& a : q.answers)
            if (a.isCorrect) res.correctAnswer = a.text;

        if (!answered) {
            res.wasCorrect   = false;
            res.chosenAnswer = "(no answer — timed out)";
            cout << "  ✗  Correct answer: " << res.correctAnswer << "\n";
        } else {
            res.chosenAnswer = q.answers[choice - 1].text;
            res.wasCorrect   = q.answers[choice - 1].isCorrect;
            if (res.wasCorrect) {
                totalScore += q.points;
                cout << "\n  ✓  Correct! +" << q.points << " pts\n";
            } else {
                cout << "\n  ✗  Wrong!  Correct: " << res.correctAnswer << "\n";
            }
        }

        results.push_back(res);
        cout << "\n  Score so far: " << totalScore << " / " << maxScore << "\n";
        this_thread::sleep_for(seconds(2));
    }

    return results;
}

// ─────────────────────────────────────────────
//  Results summary
// ─────────────────────────────────────────────

void showResults(const string& playerName, const vector<QuizResult>& results) {
    clearScreen();
    printHeader("  📊 RESULTS — " + playerName);

    int correct = 0, timedOut = 0;
    double totalTime = 0;
    int score = 0;
    int maxScore = (int)results.size() * 10;

    for (auto& r : results) {
        if (r.wasCorrect) { correct++; score += 10; }
        if (r.timedOut)   timedOut++;
        totalTime += r.timeTaken;
    }

    cout << "\n";
    cout << "  Total questions : " << results.size()   << "\n";
    cout << "  Correct answers : " << correct          << "\n";
    cout << "  Timed out       : " << timedOut         << "\n";
    cout << "  Score           : " << score << " / " << maxScore << "\n";
    cout << "  Accuracy        : " << fixed << setprecision(1)
         << (results.empty() ? 0.0 : 100.0 * correct / results.size()) << "%\n";
    cout << "  Total time      : " << fixed << setprecision(1) << totalTime << "s\n";

    double pct = results.empty() ? 0 : 100.0 * correct / results.size();
    cout << "\n  ";
    if      (pct == 100)      cout << "🏆 Perfect score! Outstanding!";
    else if (pct >= 80)       cout << "🥇 Excellent work!";
    else if (pct >= 60)       cout << "🥈 Good job, keep practising!";
    else if (pct >= 40)       cout << "🥉 Fair attempt — review the material.";
    else                      cout << "📚 Keep studying — you'll get there!";
    cout << "\n\n";

    printSeparator();
    cout << "  Question breakdown:\n";
    printSeparator();
    for (int i = 0; i < (int)results.size(); ++i) {
        auto& r = results[i];
        string status = r.wasCorrect ? "✓" : (r.timedOut ? "⏰" : "✗");
        cout << "  " << status << " Q" << (i + 1) << ": "
             << r.questionText.substr(0, 42)
             << (r.questionText.size() > 42 ? "..." : "") << "\n";
        if (!r.wasCorrect)
            cout << "      Correct: " << r.correctAnswer << "\n";
    }
    printSeparator();
}

// ─────────────────────────────────────────────
//  Leaderboard
// ─────────────────────────────────────────────

void showLeaderboard(vector<LeaderboardEntry>& board) {
    sort(board.begin(), board.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.timeTaken < b.timeTaken;   // faster wins ties
    });

    printHeader("  🏆 LEADERBOARD");
    cout << "\n";
    cout << left << setw(4) << "#"
         << setw(20) << "Player"
         << setw(10) << "Score"
         << setw(10) << "Questions"
         << "Time\n";
    printSeparator('-');

    for (int i = 0; i < (int)board.size(); ++i) {
        auto& e = board[i];
        string medal = (i == 0) ? "🥇" : (i == 1) ? "🥈" : (i == 2) ? "🥉" : "  ";
        cout << left << setw(4) << (medal + " " + to_string(i + 1))
             << setw(20) << e.playerName
             << setw(10) << (to_string(e.score) + " pts")
             << setw(10) << e.totalQuestions
             << fixed << setprecision(1) << e.timeTaken << "s\n";
    }
    cout << "\n";
}

// ─────────────────────────────────────────────
//  Main menu
// ─────────────────────────────────────────────

int main() {
    vector<Question>         bank        = buildQuestionBank();
    vector<LeaderboardEntry> leaderboard;

    while (true) {
        clearScreen();
        printHeader("  🎮 C++ QUIZ APPLICATION");
        cout << "\n";
        cout << "  [1] Start Quiz\n";
        cout << "  [2] View Leaderboard\n";
        cout << "  [3] About\n";
        cout << "  [4] Quit\n\n";
        cout << "  Choice: ";

        int menuChoice;
        if (!(cin >> menuChoice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (menuChoice == 1) {
            // ── Get player name ──────────────────────────
            clearScreen();
            printHeader("  NEW GAME");
            string name;
            cout << "\n  Enter your name: ";
            cin >> name;

            // ── Choose difficulty / number of questions ──
            cout << "\n  How many questions?\n";
            cout << "  [1] Quick  (5 questions)\n";
            cout << "  [2] Normal (8 questions)\n";
            cout << "  [3] Full   (10 questions)\n";
            cout << "  Choice: ";
            int diff;
            cin >> diff;
            int numQ = (diff == 1) ? 5 : (diff == 2) ? 8 : 10;
            numQ = min(numQ, (int)bank.size());

            // ── Run quiz ─────────────────────────────────
            auto qStart   = steady_clock::now();
            auto results  = runQuiz(name, bank, numQ);
            double elapsed = duration_cast<milliseconds>(
                                 steady_clock::now() - qStart).count() / 1000.0;

            // ── Tally score ───────────────────────────────
            int score = 0;
            for (auto& r : results) if (r.wasCorrect) score += 10;

            // ── Show results ──────────────────────────────
            showResults(name, results);

            // ── Save to leaderboard ───────────────────────
            leaderboard.push_back({name, score, numQ, elapsed});

            cout << "\n  Press Enter to return to menu...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();

        } else if (menuChoice == 2) {
            clearScreen();
            if (leaderboard.empty()) {
                printHeader("  LEADERBOARD");
                cout << "\n  No scores yet — play a game first!\n\n";
            } else {
                showLeaderboard(leaderboard);
            }
            cout << "  Press Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();

        } else if (menuChoice == 3) {
            clearScreen();
            printHeader("  ABOUT");
            cout << "\n  C++ Quiz Application\n";
            cout << "  ─────────────────────────────────────────\n";
            cout << "  Features:\n";
            cout << "   • Multiple-choice questions (1–4 options)\n";
            cout << "   • Per-question countdown timer\n";
            cout << "   • Automatic scoring system\n";
            cout << "   • Detailed results breakdown\n";
            cout << "   • In-session leaderboard with rankings\n";
            cout << "   • Three difficulty modes\n\n";
            cout << "  Extend by adding more questions to\n";
            cout << "  the buildQuestionBank() function.\n\n";
            cout << "  Press Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();

        } else if (menuChoice == 4) {
            clearScreen();
            printCentered("Thanks for playing! 👋");
            cout << "\n";
            break;
        }
    }

    return 0;
}