#include <openssl/sha.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

string sha256(const string& text) {
    unsigned char result[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(text.data()), text.size(), result);
    ostringstream out;
    for (unsigned char c : result) out << hex << setw(2) << setfill('0') << (int)c;
    return out.str();
}

string merkleRoot(vector<string> txs) {
    if (txs.empty()) return sha256("");
    for (string& tx : txs) tx = sha256(tx);
    while (txs.size() > 1) {
        if (txs.size() % 2 != 0) txs.push_back(txs.back());
        vector<string> next;
        for (size_t i = 0; i < txs.size(); i += 2)
            next.push_back(sha256(txs[i] + txs[i + 1]));
        txs = move(next);
    }
    return txs[0];
}

struct Validator { string name; unsigned int stake; };
struct Block {
    size_t index;
    string previousHash;
    vector<string> transactions;
    string merkle;
    string consensus;
    unsigned int difficulty;
    unsigned long long nonce;
    string validator;
    string hash;
};

string calculateHash(const Block& b) {
    // Donnees deterministes: modifier un champ modifie le hash.
    return sha256(to_string(b.index) + "|" + b.previousHash + "|" + b.merkle +
                  "|" + b.consensus + "|" + to_string(b.difficulty) + "|" +
                  to_string(b.nonce) + "|" + b.validator);
}

bool hasDifficulty(const string& hash, unsigned int difficulty) {
    return hash.compare(0, difficulty, string(difficulty, '0')) == 0;
}

class Blockchain {
    vector<Block> chain;
    vector<Validator> validators;
    mt19937 rng;
public:
    explicit Blockchain(vector<Validator> vals) : validators(move(vals)), rng(42) {
        Block genesis{0, "0", {"Bloc genesis"}, "", "GENESIS", 0, 0, "", ""};
        genesis.merkle = merkleRoot(genesis.transactions);
        genesis.hash = calculateHash(genesis);
        chain.push_back(genesis);
    }

    string selectValidator() {
        unsigned long long total = 0;
        for (const auto& v : validators) total += v.stake;
        if (!total) throw runtime_error("Aucun stake disponible");
        uniform_int_distribution<unsigned long long> dist(1, total);
        unsigned long long draw = dist(rng), current = 0;
        for (const auto& v : validators) {
            current += v.stake;
            if (draw <= current) return v.name;
        }
        throw runtime_error("Selection impossible");
    }

    double addBlock(const vector<string>& txs, const string& mode, unsigned int difficulty = 0) {
        if (mode != "PoW" && mode != "PoS") throw invalid_argument("Mode inconnu");
        if (mode == "PoW" && (difficulty < 1 || difficulty > 6))
            throw invalid_argument("Difficulte PoW entre 1 et 6");
        Block b{chain.size(), chain.back().hash, txs, merkleRoot(txs), mode,
                mode == "PoW" ? difficulty : 0, 0, "", ""};
        auto start = Clock::now();
        if (mode == "PoS") b.validator = selectValidator();
        b.hash = calculateHash(b);
        if (mode == "PoW") {
            while (!hasDifficulty(b.hash, b.difficulty)) {
                ++b.nonce;
                b.hash = calculateHash(b);
            }
        }
        auto stop = Clock::now();
        chain.push_back(b);
        double ms = chrono::duration<double, milli>(stop - start).count();
        cout << "Bloc " << b.index << " [" << mode << "]"
             << " | nonce=" << b.nonce << " | validateur="
             << (b.validator.empty() ? "-" : b.validator)
             << " | temps=" << fixed << setprecision(3) << ms << " ms\n";
        cout << "  previousHash=" << b.previousHash << "\n";
        cout << "  merkleRoot  =" << b.merkle << "\n";
        cout << "  hash        =" << b.hash << "\n";
        return ms;
    }

    bool isValid() const {
        for (size_t i = 0; i < chain.size(); ++i) {
            const Block& b = chain[i];
            if (b.merkle != merkleRoot(b.transactions)) return false;
            if (b.hash != calculateHash(b)) return false;
            if (i == 0) {
                if (b.previousHash != "0" || b.consensus != "GENESIS") return false;
                continue;
            }
            if (b.index != i || b.previousHash != chain[i - 1].hash) return false;
            if (b.consensus == "PoW") {
                if (b.difficulty < 1 || b.difficulty > 6 || !b.validator.empty() ||
                    !hasDifficulty(b.hash, b.difficulty)) return false;
            } else if (b.consensus == "PoS") {
                if (b.difficulty != 0 || b.nonce != 0) return false;
                bool found = false;
                for (const auto& v : validators)
                    if (v.name == b.validator && v.stake > 0) found = true;
                if (!found) return false;
            } else return false;
        }
        return true;
    }

    void tamper(size_t index) {
        if (index > 0 && index < chain.size())
            chain[index].transactions[0] = "Transaction modifiee frauduleusement";
    }
};

int main() {
    vector<Validator> validators{{"Alice", 50}, {"Bob", 30}, {"Charlie", 20}};
    const vector<vector<string>> transactions = {
        {"Alice -> Bob : 10", "Bob -> Charlie : 5"},
        {"Charlie -> David : 2", "David -> Alice : 1"},
        {"Alice -> Charlie : 4", "Bob -> David : 3"}
    };
    const unsigned int difficulty = 4;

    cout << "=== Blockchain avec Proof of Work ===\n";
    Blockchain powChain(validators);
    double powTotal = 0;
    for (const auto& txs : transactions) powTotal += powChain.addBlock(txs, "PoW", difficulty);
    cout << "Chaine PoW valide : " << (powChain.isValid() ? "OUI" : "NON") << "\n";

    cout << "\n=== Blockchain avec Proof of Stake (simulation) ===\n";
    Blockchain posChain(validators);
    double posTotal = 0;
    for (const auto& txs : transactions) posTotal += posChain.addBlock(txs, "PoS");
    cout << "Chaine PoS valide : " << (posChain.isValid() ? "OUI" : "NON") << "\n";

    cout << "\n=== Comparaison des temps (3 blocs, hors creation du genesis) ===\n";
    cout << "PoW : " << powTotal << " ms\nPoS : " << posTotal << " ms\n";
    cout << "Plus rapide dans ce test : " << (powTotal < posTotal ? "PoW" : "PoS") << "\n";

    cout << "\n=== Test d'integrite ===\n";
    cout << "Avant modification : " << (powChain.isValid() ? "VALIDE" : "INVALIDE") << "\n";
    powChain.tamper(1);
    cout << "Apres modification : " << (powChain.isValid() ? "VALIDE" : "INVALIDE") << "\n";
    return 0;
}
