#include <bits/stdc++.h>
using namespace std;

static string read_all_stdin() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    std::ostringstream oss;
    string line;
    // Read raw bytes preserving newlines
    while (true) {
        char buf[4096];
        cin.read(buf, sizeof(buf));
        streamsize g = cin.gcount();
        if (g > 0) oss.write(buf, g);
        if (!cin) break;
    }
    return oss.str();
}

static string strip_comments(const string &s) {
    // Remove ';' to end-of-line comments
    string out;
    out.reserve(s.size());
    bool in_comment = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (!in_comment) {
            if (c == ';') {
                in_comment = true;
            } else {
                out.push_back(c);
            }
        } else {
            // skip until newline
            if (c == '\n' || c == '\r') {
                in_comment = false;
                out.push_back(c);
            }
        }
    }
    return out;
}

static vector<string> tokenize(const string &s) {
    vector<string> tokens;
    tokens.reserve(s.size()/2);
    auto is_space = [](char c){ return c==' '||c=='\n'||c=='\t'||c=='\r' || c=='\f'||c=='\v'; };
    auto is_digit = [](char c){ return c>='0' && c<='9'; };
    auto is_ident_char = [](char c){
        // visible ASCII except parentheses and semicolon
        return c>=33 && c<=126 && c!='(' && c!=')' && c!=';';
    };
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (is_space(c)) { ++i; continue; }
        if (c == '(' || c == ')') {
            tokens.emplace_back(1, c);
            ++i; continue;
        }
        // read an identifier-like chunk until space or paren
        size_t j = i;
        while (j < n) {
            char cj = s[j];
            if (is_space(cj) || cj=='(' || cj==')') break;
            ++j;
        }
        if (j>i) {
            string t = s.substr(i, j-i);
            tokens.push_back(t);
        }
        i = j;
    }
    return tokens;
}

static bool is_integer_token(const string &t) {
    if (t.empty()) return false;
    size_t i = 0;
    if (t[0]=='-' || t[0]=='+') {
        if (t.size()==1) return false;
        i = 1;
    }
    for (; i < t.size(); ++i) if (!(t[i]>='0'&&t[i]<='9')) return false;
    return true;
}

static vector<string> normalize_tokens(const vector<string> &tok) {
    vector<string> out;
    out.reserve(tok.size());
    static const unordered_set<string> keywords = {
        "function","block","set","if","for","return",
        "+","-","*","/","%","<",">","<=",">=","==","!=","||","&&","!",
        "scan","print","array.create","array.scan","array.print","array.get","array.set",
        "(", ")"
    };
    for (const string &t : tok) {
        if (t == "(" || t == ")") { out.push_back(t); continue; }
        if (keywords.count(t)) { out.push_back(t); continue; }
        if (is_integer_token(t)) { out.push_back("NUM"); continue; }
        // normalize identifiers and other symbols to ID
        out.push_back("ID");
    }
    return out;
}

static double jaccard_kgrams(const vector<string> &tokens, int k) {
    if (k <= 0) return 0.0;
    unordered_set<string> A;
    if ((int)tokens.size() >= k) {
        for (size_t i = 0; i + k <= tokens.size(); ++i) {
            string key;
            key.reserve(k*4);
            for (int j = 0; j < k; ++j) {
                key += tokens[i+j];
                key.push_back('\x1F'); // unit separator
            }
            A.insert(std::move(key));
        }
    }
    return (double)A.size(); // helper overload
}

static double jaccard_similarity(const vector<string> &t1, const vector<string> &t2, int k) {
    unordered_set<string> A, B;
    if ((int)t1.size() >= k) {
        for (size_t i = 0; i + k <= t1.size(); ++i) {
            string key;
            for (int j = 0; j < k; ++j) {
                key += t1[i+j];
                key.push_back('\x1F');
            }
            A.insert(std::move(key));
        }
    }
    if ((int)t2.size() >= k) {
        for (size_t i = 0; i + k <= t2.size(); ++i) {
            string key;
            for (int j = 0; j < k; ++j) {
                key += t2[i+j];
                key.push_back('\x1F');
            }
            B.insert(std::move(key));
        }
    }
    if (A.empty() && B.empty()) return 0.0;
    size_t inter = 0;
    if (A.size() < B.size()) {
        for (auto &x : A) if (B.count(x)) ++inter;
    } else {
        for (auto &x : B) if (A.count(x)) ++inter;
    }
    size_t uni = A.size() + B.size() - inter;
    if (uni == 0) return 0.0;
    return (double)inter / (double)uni;
}

int main() {
    string input = read_all_stdin();
    // Decide mode by counting standalone token "endprogram"
    // Split on whitespace to count; being lenient: any occurrence of "endprogram" delimited by non-identifier chars
    // We'll scan tokens from a simple whitespace split, plus remove comments first to avoid ';endprogram' cases.
    string no_comments = strip_comments(input);
    // Quick scan for endprogram tokens using tokenization that splits on parens and spaces
    vector<string> raw_tokens = tokenize(no_comments);
    int end_count = 0;
    for (const string &t : raw_tokens) if (t == "endprogram") ++end_count;

    if (end_count >= 2) {
        // Anticheat mode: extract two programs up to the first two endprogram tokens
        // Build strings for program1 and program2 by iterating tokens and reconstructing with spaces
        string prog1, prog2;
        int seen = 0;
        for (size_t i = 0; i < raw_tokens.size(); ++i) {
            const string &t = raw_tokens[i];
            if (t == "endprogram") {
                ++seen;
                if (seen >= 2) { ++i; // move past second endprogram
                    break; }
                continue; // do not include the delimiter
            }
            if (seen == 0) {
                if (!prog1.empty()) prog1.push_back(' ');
                prog1 += t;
            } else if (seen == 1) {
                if (!prog2.empty()) prog2.push_back(' ');
                prog2 += t;
            }
        }
        // Tokenize and normalize each program
        auto t1 = normalize_tokens(tokenize(strip_comments(prog1)));
        auto t2 = normalize_tokens(tokenize(strip_comments(prog2)));
        // Combine multiple k values for robustness
        double s2 = jaccard_similarity(t1, t2, 2);
        double s3 = jaccard_similarity(t1, t2, 3);
        double s4 = jaccard_similarity(t1, t2, 4);
        // Weighted average leaning toward trigrams
        double s = (0.2*s2 + 0.5*s3 + 0.3*s4);
        if (!(s >= 0.0)) s = 0.0; // handle NaN
        if (s < 0.0) s = 0.0;
        if (s > 1.0) s = 1.0;
        cout.setf(std::ios::fixed); cout<<setprecision(6)<<s; 
        if (!cout.good()) return 0;
        return 0;
    } else {
        // Cheat mode: rename identifiers and canonicalize formatting
        string stripped = strip_comments(input);
        auto toks = tokenize(stripped);
        // Builtin and syntax tokens to keep unchanged
        static const unordered_set<string> keep = {
            "(", ")",
            "function","block","set","if","for","return",
            "+","-","*","/","%","<",">","<=",">=","==","!=","||","&&","!",
            "scan","print","array.create","array.scan","array.print","array.get","array.set"
        };
        // Also preserve entrypoint name 'main'
        unordered_set<string> preserve_id = {"main"};
        unordered_map<string,string> mp;
        int counter = 1;
        auto gen_name = [&](){ return string("v") + to_string(counter++); };
        // Produce renamed token stream
        vector<string> out_toks;
        out_toks.reserve(toks.size());
        for (const string &t : toks) {
            if (t == "(" || t == ")") {
                out_toks.push_back(t);
            } else if (keep.count(t)) {
                out_toks.push_back(t);
            } else if (is_integer_token(t)) {
                out_toks.push_back(t);
            } else {
                if (preserve_id.count(t)) { out_toks.push_back(t); continue; }
                auto it = mp.find(t);
                if (it == mp.end()) {
                    string name;
                    do { name = gen_name(); } while (keep.count(name));
                    it = mp.emplace(t, name).first;
                }
                out_toks.push_back(it->second);
            }
        }
        // Reconstruct with canonical spacing: no space after '(', no space before ')', single spaces elsewhere
        string out;
        out.reserve(stripped.size());
        auto ends_with_space_like = [&](const string &s){
            if (s.empty()) return true;
            char c = s.back();
            return c==' ' || c=='\n' || c=='\t' || c=='(';
        };
        for (const string &t : out_toks) {
            if (t == "(") {
                out.push_back('(');
            } else if (t == ")") {
                // remove any trailing space before ')'
                while (!out.empty() && out.back()==' ') out.pop_back();
                out.push_back(')');
            } else {
                if (!ends_with_space_like(out)) out.push_back(' ');
                out += t;
            }
        }
        // Ensure trailing newline for cleanliness
        if (out.empty() || out.back()!='\n') out.push_back('\n');
        cout << out;
        return 0;
    }
}
