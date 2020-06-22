#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <boost/filesystem.hpp>
#include <cstdio>
#include <string_view>
#include <exception>
#include <execution>

#define LOG(s) cout << s << endl

using namespace std;

struct {
    int seq_col = 2;
    int pos_col = 3;
    char sep = '\t';
    string input;
    string output;
    size_t max_open_files = 32;
    size_t initial_run_size = 10'000'000;
} CONFIG;

size_t MERGE_ROUNDS = 0;
vector<string> temp_files;
vector<string> runs_files;
vector<string> next_runs;

size_t find_nth(const string& src, const char qry, size_t n) {
    if (n == 0) {
        return 0;
    }

    size_t i = src.find(qry);

    int j;
    for (j = 1; j < n and i != string::npos; j++) {
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


struct file_val {
    file_val (string f) {
        filename = f;
        input = make_shared<ifstream> (f);
        next_line();
    }

    bool next_line() {
        bool ret = static_cast<bool> (getline(*input, line));
        if (ret) {
            seq = get_seq(line);
            pos = get_pos(line);
        }

        return ret;
    }

    shared_ptr<ifstream> input;
    string line;
    string seq;
    size_t pos;
    string filename;
};

bool cmp(const string &a, const string &b) {
    auto seq_a = get_seq(a);
    auto seq_b = get_seq(b);

    bool seq_lt = seq_a < seq_b;
    bool seq_eq = seq_a == seq_b;
    bool pos_lt = get_pos(a) < get_pos(b);
    return seq_lt or (seq_eq and pos_lt);
}

bool run_cmp(const string &a, const string &b) {
    return cmp(a, b);
}

bool heap_cmp(const file_val &a, const file_val &b) {
    bool seq_lt = b.seq < a.seq;
    bool seq_eq = b.seq == a.seq;
    bool pos_lt = b.pos < a.pos;
    return seq_lt or (seq_eq and pos_lt);
}

template <typename T> class Heap {
public:
    void push(T val) {
        x_.push_back(val);
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
    LOG(temp_file);
    temp_files.push_back(temp_file.native());

    return(temp_file.native());
}

void split_runs(ifstream &input) {
    string read_buf;
    vector<string> sort_buffer;
    sort_buffer.reserve(CONFIG.initial_run_size);

    while (getline(input, read_buf)) {
        if (sort_buffer.size() >= CONFIG.initial_run_size) {
            string out_path = new_tmpfile();
            ofstream out_file(out_path);
            temp_files.push_back(out_path);

            sort(execution::par, sort_buffer.begin(), sort_buffer.end(), run_cmp);
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

    LOG("merging runs...");
    MERGE_ROUNDS++;
    Heap<file_val> heap;

    while (runs_files.size() > 0) {
        while (heap.size() <= CONFIG.max_open_files and runs_files.size() > 0) {
            string f = runs_files.back();
            runs_files.pop_back();

            heap.push(file_val(f) );
        }

        next_runs.push_back(new_tmpfile());

        ofstream merge_file(next_runs.back());

        while (heap.size() > 0) {
            auto x = heap.pop();
            merge_file << x.line << "\n";

            if (x.next_line()) {
                heap.push(x);
            } else {
                remove(x.filename.c_str());
            }
        }
    }

    while (next_runs.size() > 0) {
        runs_files.push_back(next_runs.back());
        next_runs.pop_back();
    }
    
    merge_runs();
}

void move_final_result() {
    LOG("moving results...");
    boost::filesystem::rename(runs_files.back(), CONFIG.output);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        throw runtime_error("insufficient arguments");
    }

    LOG(argv[1]);
    LOG(argv[2]);
    CONFIG.input = argv[1];
    CONFIG.output = argv[2];

    atexit(clean_up_files);

    int length = 8192;
    char buffer[length];
    ifstream input;
    input.rdbuf()->pubsetbuf(buffer, length);
    input.open(CONFIG.input);

    split_runs(input);
    merge_runs();

    move_final_result();
    LOG("merge rounds: " << MERGE_ROUNDS);
}
