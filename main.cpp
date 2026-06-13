#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <ctime>
#include <windows.h>

using namespace std;

void sleepSeconds(int s) { Sleep(s * 1000); }
void clearScreen() { system("cls"); }

struct Answer {
    string text;
    bool isCorrect;
};

struct Question {
    string text;
    vector<Answer> answers;
    int timeLimitSeconds;
    int points;
};

struct QuizResult {
    string questionText;
    string chosenAnswer;
    string correctAnswer;
    bool wasCorrect;
    bool timedOut;
};

struct LeaderboardEntry {
    string playerName;
    int score;
    int totalQuestions;
    double timeTaken;
};

void printLine(char ch, int len) {
    for (int i = 0; i < len; i++) cout << ch;
    cout << "\n";
}

void printHeader(const string& title) {
    printLine('=', 60);
    int pad = (60 - (int)title.size()) / 2;
    if (pad > 0) cout << string(pad, ' ');
    cout << title << "\n";
    printLine('=', 60);
}

vector<Question> buildQuestions() {
    return {
        {
            "What does OOP stand for in C++?",
            {
                {"Object-Oriented Programming", true},
                {"Open Object Protocol",        false},
                {"Operator Overloading Process",false},
                {"Organised Output Parsing",    false}
            }, 15, 10
        },
        {
            "Which keyword allocates memory dynamically in C++?",
            {
                {"malloc", false},
                {"alloc",  false},
                {"new",    true},
                {"create", false}
            }, 15, 10
        },
        {
            "What is the output of: cout << 5 / 2 (both integers)?",
            {
                {"2.5", false},
                {"2",   true},
                {"3",   false},
                {"0",   false}
            }, 20, 10
        },
        {
            "Which header is needed for cout and cin?",
            {
                {"<stdio.h>",  false},
                {"<conio.h>",  false},
                {"<iostream>", true},
                {"<string.h>", false}
            }, 15, 10
        },
        {
            "What does a pointer store in C++?",
            {
                {"A direct value",         false},
                {"A memory address",       true},
                {"A function reference",   false},
                {"A type definition",      false}
            }, 20, 10
        },
        {
            "What does the 'virtual' keyword enable?",
            {
                {"Memory allocation",    false},
                {"Runtime polymorphism", true},
                {"Compile-time binding", false},
                {"Operator overloading", false}
            }, 20, 10
        },
        {
            "Which of these is NOT a valid C++ loop?",
            {
                {"for",    false},
                {"while",  false},
                {"repeat", true},
                {"do",     false}
            }, 15, 10
        },
        {
            "What is the size of int on most 64-bit systems?",
            {
                {"2 bytes", false},
                {"4 bytes", true},
                {"8 bytes", false},
                {"1 byte",  false}
            }, 15, 10
        },
        {
            "What does STL stand for?",
            {
                {"Standard Type Library",     false},
                {"Static Template Library",   false},
                {"Standard Template Library", true},
                {"Structured Text Language",  false}
            }, 15, 10
        },
        {
            "Which operator accesses members through a pointer?",
            {
                {".",  false},
                {"->", true},
                {"*",  false},
                {"&",  false}
            }, 15, 10
        }
    };
}

bool askQuestion(const Question& q, QuizResult& result) {
    printLine('-', 60);
    cout << "\n  " << q.text << "\n\n";
    for (int i = 0; i < (int)q.answers.size(); i++)
        cout << "  [" << (i + 1) << "] " << q.answers[i].text << "\n";
    cout << "\n";

    for (auto& a : q.answers)
        if (a.isCorrect) result.correctAnswer = a.text;

    result.timedOut  = false;
    result.wasCorrect = false;

    clock_t start = clock();
    int choice = 0;
    int lastRemaining = -1;

    while (true) {
        int elapsed   = (int)((double)(clock() - start) / CLOCKS_PER_SEC);
        int remaining = q.timeLimitSeconds - elapsed;

        if (remaining <= 0) {
            cout << "\n\n  Time's up!\n";
            result.timedOut = true;
            return false;
        }

        if (remaining != lastRemaining) {
            lastRemaining = remaining;
            int barWidth = 20;
            int filled   = (remaining * barWidth) / q.timeLimitSeconds;
            cout << "\r  Time: " << setw(2) << remaining << "s [";
            for (int i = 0; i < barWidth; i++)
                cout << (i < filled ? '#' : '.');
            cout << "]  Answer (1-" << q.answers.size() << "): ";
            cout.flush();
        }

        // Check if there is input available using Windows API
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD events = 0;
        GetNumberOfConsoleInputEvents(hStdin, &events);

        if (events > 0) {
            string inputStr;
            cin >> inputStr;
            try {
                choice = stoi(inputStr);
                if (choice >= 1 && choice <= (int)q.answers.size()) {
                    result.chosenAnswer = q.answers[choice - 1].text;
                    result.wasCorrect   = q.answers[choice - 1].isCorrect;
                    return true;
                } else {
                    cout << "\n  [!] Enter a number between 1 and " << q.answers.size() << "\n";
                    lastRemaining = -1;
                }
            } catch (...) {
                cout << "\n  [!] Invalid input. Enter a number.\n";
                lastRemaining = -1;
            }
        }

        Sleep(100);
    }
}

void showResults(const string& name, const vector<QuizResult>& results) {
    clearScreen();
    printHeader("RESULTS - " + name);

    int correct = 0, timedOut = 0, score = 0;
    int maxScore = (int)results.size() * 10;

    for (auto& r : results) {
        if (r.wasCorrect) { correct++; score += 10; }
        if (r.timedOut)   timedOut++;
    }

    double accuracy = results.empty() ? 0 : 100.0 * correct / results.size();

    cout << "\n";
    cout << "  Total questions : " << results.size()               << "\n";
    cout << "  Correct         : " << correct                      << "\n";
    cout << "  Wrong           : " << (int)results.size() - correct - timedOut << "\n";
    cout << "  Timed out       : " << timedOut                     << "\n";
    cout << "  Score           : " << score << " / " << maxScore   << "\n";
    cout << "  Accuracy        : " << fixed << setprecision(1) << accuracy << "%\n";

    cout << "\n  ";
    if      (accuracy == 100) cout << "*** Perfect score! Outstanding! ***";
    else if (accuracy >= 80)  cout << "*** Excellent work! ***";
    else if (accuracy >= 60)  cout << "** Good job! Keep practising. **";
    else if (accuracy >= 40)  cout << "* Fair attempt. Review the material. *";
    else                      cout << "Keep studying - you will get there!";
    cout << "\n\n";

    printLine('-', 60);
    cout << "  Breakdown:\n";
    printLine('-', 60);
    for (int i = 0; i < (int)results.size(); i++) {
        auto& r = results[i];
        string status = r.wasCorrect ? "[OK]" : (r.timedOut ? "[TM]" : "[X] ");
        string qShort = r.questionText.substr(0, 42);
        if (r.questionText.size() > 42) qShort += "...";
        cout << "  " << status << " Q" << (i + 1) << ": " << qShort << "\n";
        if (!r.wasCorrect)
            cout << "            Correct: " << r.correctAnswer << "\n";
    }
    printLine('-', 60);
}

void showLeaderboard(vector<LeaderboardEntry>& board) {
    sort(board.begin(), board.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.score != b.score ? a.score > b.score : a.timeTaken < b.timeTaken;
    });

    clearScreen();
    printHeader("LEADERBOARD");
    cout << "\n";
    cout << left << setw(5) << "#"
         << setw(18) << "Player"
         << setw(10) << "Score"
         << setw(10) << "Qs"
         << "Time\n";
    printLine('-', 60);
    for (int i = 0; i < (int)board.size(); i++) {
        auto& e = board[i];
        cout << left << setw(5)  << (to_string(i + 1) + ".")
             << setw(18) << e.playerName
             << setw(10) << (to_string(e.score) + " pts")
             << setw(10) << e.totalQuestions
             << fixed << setprecision(1) << e.timeTaken << "s\n";
    }
    cout << "\n";
}

int main() {
    vector<Question>         bank = buildQuestions();
    vector<LeaderboardEntry> leaderboard;

    while (true) {
        clearScreen();
        printHeader("C++ QUIZ APPLICATION");
        cout << "\n";
        cout << "  [1] Start Quiz\n";
        cout << "  [2] View Leaderboard\n";
        cout << "  [3] Quit\n\n";
        cout << "  Choice: ";

        int menuChoice = 0;
        if (!(cin >> menuChoice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (menuChoice == 1) {
            clearScreen();
            printHeader("NEW GAME");
            string name;
            cout << "\n  Enter your name: ";
            cin >> name;

            cout << "\n  Select number of questions:\n";
            cout << "  [1] Quick  - 5 questions\n";
            cout << "  [2] Normal - 8 questions\n";
            cout << "  [3] Full   - 10 questions\n";
            cout << "  Choice: ";
            int diff = 0;
            cin >> diff;
            int numQ = (diff == 1) ? 5 : (diff == 2) ? 8 : 10;
            numQ = min(numQ, (int)bank.size());

            vector<QuizResult> results;
            int totalScore = 0;
            int maxScore   = numQ * 10;

            clearScreen();
            printHeader("QUIZ START - Good Luck, " + name + "!");
            cout << "\n  " << numQ << " questions  |  Max score: " << maxScore << " pts\n\n";
            sleepSeconds(2);

            clock_t gameStart = clock();

            for (int i = 0; i < numQ; i++) {
                clearScreen();
                printLine('-', 60);
                cout << "  Question " << (i + 1) << " / " << numQ
                     << "  |  +" << bank[i].points << " pts"
                     << "  |  Time limit: " << bank[i].timeLimitSeconds << "s\n";

                QuizResult res;
                res.questionText = bank[i].text;
                res.wasCorrect   = false;
                res.timedOut     = false;
                res.chosenAnswer = "";

                bool answered = askQuestion(bank[i], res);

                if (!answered) {
                    cout << "  Correct answer was: " << res.correctAnswer << "\n";
                } else if (res.wasCorrect) {
                    totalScore += bank[i].points;
                    cout << "\n\n  [OK] Correct! +" << bank[i].points << " pts\n";
                } else {
                    cout << "\n\n  [X] Wrong! Correct answer: " << res.correctAnswer << "\n";
                }

                results.push_back(res);
                cout << "\n  Score: " << totalScore << " / " << maxScore << "\n";
                sleepSeconds(2);
            }

            double totalTime = (double)(clock() - gameStart) / CLOCKS_PER_SEC;

            showResults(name, results);
            leaderboard.push_back({name, totalScore, numQ, totalTime});

            cout << "\n  Press Enter to return to menu...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();

        } else if (menuChoice == 2) {
            if (leaderboard.empty()) {
                clearScreen();
                printHeader("LEADERBOARD");
                cout << "\n  No scores yet. Play a game first!\n\n";
            } else {
                showLeaderboard(leaderboard);
            }
            cout << "  Press Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();

        } else if (menuChoice == 3) {
            clearScreen();
            cout << "\n  Thanks for playing! Goodbye.\n\n";
            break;
        }
    }

    return 0;
}
