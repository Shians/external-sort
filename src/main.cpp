#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <boost/filesystem.hpp>
#include <cstdio>
#include <string_view>

#define LOG(s) cout << s << endl

using namespace std;

struct {
    int seq_col = 1;
    int pos_col = 2;
    char sep = '\t';
} CONFIG;

uint RUN_SIZE = 200;
vector<string> temp_files;
vector<string> runs_files;

struct file_val {
    file_val (string f) {
        input = make_shared<ifstream> (f);
        next_line();
    }

    bool next_line() {
        return static_cast<bool>(getline(*input, line));
    }

    shared_ptr<ifstream> input;
    string line;
};

size_t find_nth(const string& src, const char qry, size_t n) {
    if (n == 0) {
        return 0;
    }

    size_t i = src.find(qry);

    int j;
    for (j = 1; j < n && i != string::npos; j++) {
        i = src.find(qry, i + 1);
    }

    if (j == n) {
        return i;
    } else {
        return string::npos;
    }
}

string get_seq(const string &s) {
    size_t start = find_nth(s, CONFIG.sep, CONFIG.seq_col - 1);
    size_t end = find_nth(s, CONFIG.sep, CONFIG.seq_col);
    size_t count = end == string::npos ? string::npos : end - start;

    return s.substr(start, count);
}

unsigned long get_pos(const string &s) {
    size_t start = find_nth(s, CONFIG.sep, CONFIG.pos_col - 1);
    size_t end = find_nth(s, CONFIG.sep, CONFIG.pos_col);
    size_t count = end == string::npos ? string::npos : end - start;
    return stoi(s.substr(start, count));
}

bool cmp(const string &a, const string &b) {
    bool seq_lt = get_seq(a) < get_seq(b);
    bool seq_eq = get_seq(a) == get_seq(b);
    bool pos_lt = get_pos(a) < get_pos(b);
    return seq_lt || (seq_eq && pos_lt);
}

bool run_cmp(const string &a, const string &b) {
    return cmp(a, b);
}

bool heap_cmp(const file_val &a, const file_val &b) {
    bool seq_lt = get_seq(a.line) > get_seq(b.line);
    bool seq_eq = get_seq(a.line) == get_seq(b.line);
    bool pos_lt = get_pos(a.line) > get_pos(b.line);
    return seq_lt || (seq_eq && pos_lt);
}

template <typename T> class Heap {
public:
    void push(T val) {
        x_.emplace_back(val);
        push_heap(x_.begin(), x_.end(), heap_cmp);
    }

    T pop(void) {
        pop_heap(x_.begin(), x_.end(), heap_cmp);
        T ret = x_.back();
        x_.pop_back();
        return(ret);
    }

    size_t size() {
        return(x_.size());
    }
private:
    vector<T> x_;
};

void clean_up_files() {
    for (string const& f : temp_files) {
        remove(f.c_str());
    }
}

string new_tmpfile() {
    auto temp_file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    cout << temp_file << "\n";
    temp_files.push_back(temp_file.native());

    return(temp_file.native());
}

void split_runs(ifstream &input) {

    string read_buf;
    vector<string> sort_buffer;

    while (getline(input, read_buf)) {
        if (sort_buffer.size() >= RUN_SIZE) {
            string out_path = new_tmpfile();
            ofstream out_file(out_path);
            temp_files.push_back(out_path);

            sort(sort_buffer.begin(), sort_buffer.end(), run_cmp);
            for (auto const& line : sort_buffer) {
                out_file << line << "\n";
            }

            runs_files.push_back(out_path);
            sort_buffer.clear();
        }

        sort_buffer.push_back(read_buf);
    }

    if (sort_buffer.size() > 0) {
        string out_path = new_tmpfile();
        ofstream out_file(out_path);
        temp_files.push_back(out_path);

        sort(sort_buffer.begin(), sort_buffer.end(), run_cmp);
        for (auto const& line : sort_buffer) {
            out_file << line << "\n";
        }

        runs_files.push_back(out_path);
        sort_buffer.clear();
    }
}

void merge_runs() {
    if (runs_files.size() == 1) {
        return;
    }

    Heap<file_val> heap;

    while (runs_files.size() > 0) {
        string f = runs_files.back();
        runs_files.pop_back();

        heap.push(file_val(f) );
    }

    while (heap.size() > 0) {
        auto x = heap.pop();
        cout << x.line << "\n";

        if (x.next_line()) {
            heap.push(x);
        }
    }

    // merge_runs();
}

int main(int argc, char* argv[]) {
    atexit(clean_up_files);

    int length = 8192;
    char buffer[length];
    ifstream input;
    input.rdbuf()->pubsetbuf(buffer, length);
    input.open("file.txt");

    cout << "splitting runs..." << "\n";
    split_runs(input);
    cout << "merging runs..." << "\n";
    merge_runs();
}
