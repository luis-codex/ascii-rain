#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <unistd.h>
#include <ncurses.h>
#include <iomanip>
#include <sstream>

bool userResized = false;
bool slowerDrops = false;
long frameDelay = 80; 

class Drop {
public:
    int w;
    int h;
    int speed;
    int color;
    char shape;

    Drop() {
        w = randomInRange(0, COLS - 1);
        h = randomInRange(0, LINES - 1);

        if (slowerDrops) {
            speed = randomInRange(1, 3);
            shape = (speed < 2) ? '|' : ':';
        } else {
            speed = randomInRange(1, 6);
            shape = (speed < 3) ? '|' : ':';
        }

        int x = speed;
        color = static_cast<int>((0.0416 * (x - 4) * (x - 3) * (x - 2) - 4) * (x - 1) + 255);
    }

    void fall() {
        h += speed;
        if (h >= LINES)
            h = randomInRange(0, 10);
    }

    void show() const {
        attron(COLOR_PAIR(color));
        mvaddch(h, w, shape);
    }

    static int randomInRange(int min, int max) {
        max -= 1;
        return min + rand() / (RAND_MAX / (max - min + 1) + 1);
    }
};

class DropVector {
public:
    std::vector<Drop> drops;

    DropVector(int cap) {
        drops.reserve(cap);
        for (int i = 0; i < cap; ++i) {
            drops.push_back(Drop());
        }
    }

    void resize(int newCap) {
        drops.clear();
        drops.reserve(newCap);
        for (int i = 0; i < newCap; ++i) {
            drops.push_back(Drop());
        }
    }

    Drop& getAt(int pos) {
        if (pos < 0 || pos >= drops.size()) {
            throw std::out_of_range("Bad access");
        }
        return drops[pos];
    }
};

void initCurses() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, 1);
    curs_set(0);

    if (curs_set(0) == 1)
        throw std::runtime_error("Terminal emulator lacks capabilities. (Can't hide Cursor).");

    timeout(0);
    signal(SIGWINCH, [](int){ userResized = true; });

    if (has_colors() && can_change_color()) {
        use_default_colors();
        start_color();
        for (short i = 0; i < COLORS; ++i)
            init_pair(i + 1, i, -1);
    } else {
        throw std::runtime_error("Terminal emulator lacks capabilities. (Can't have colors).");
    }
}

void exitCurses() {
    curs_set(1);
    clear();
    refresh();
    endwin();
}

int getNumOfDrops() {
    int nDrops = 0;
    if ((LINES < 20 && COLS > 100) || (COLS < 100 && LINES < 40)) {
        nDrops = static_cast<int>(COLS * 0.75);
        slowerDrops = true;
    } else {
        nDrops = static_cast<int>(COLS * 1.5);
        slowerDrops = false;
    }
    return nDrops;
}

void mssleep(long msec) {
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, nullptr);
}

void usage() {
    std::cout << "Usage: rain [frame delay in milliseconds]\n";
    std::cout << "No arguments required. Default frame delay is 80 ms.\n";
    std::cout << "Hit 'q' to exit.\n";
}

std::string getCurrentTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
    return oss.str();
}

void displayTime() {
    std::string timeStr = getCurrentTime();
    int x = (COLS - timeStr.length()) / 2;
    int y = LINES / 2;
    mvprintw(y, x, "%s", timeStr.c_str());
}

void handleResize(DropVector& drops) {
    endwin();
    refresh();
    clear();
    drops.resize(getNumOfDrops());
    userResized = false;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        usage();
        return 0;
    }

    if (argc == 2) {
        try {
            frameDelay = std::stol(argv[1]);
        } catch (const std::exception& e) {
            usage();
            return 1;
        }
    }

    int key;
    srand(static_cast<unsigned int>(getpid()));
    try {
        initCurses();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    int dropsTotal = getNumOfDrops();
    DropVector drops(dropsTotal);

    while (true) {
        if (userResized) {
            handleResize(drops);
            dropsTotal = getNumOfDrops();
        }

        for (int i = 0; dropsTotal && i < dropsTotal; ++i) {
            drops.getAt(i).fall();
            drops.getAt(i).show();
        }

        displayTime();
        mssleep(frameDelay);

        if ((key = wgetch(stdscr)) == 'q')
            break;

        erase();
    }

    exitCurses();
    return 0;
}
