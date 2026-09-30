// bench_trees.cpp
// Benchmark completo de AVL, Rojo-Negro (std::set) y Splay.
//
// Compilar (desde boletin-2/):
//   g++ -std=c++17 -O2 -o bench_trees src/bench_trees.cpp
//
// Uso:
//   ./bench_trees <exp> <salida.csv> [opciones]
//   exp: search | insert | delete | skew | repeat | control | all
//        (con "all", <salida> es un prefijo: genera <prefijo>_<exp>.csv)
//
// Experimentos:
//   search  : search vs n, consultas uniformes (linea base sin sesgo).
//   insert  : insert vs n (claves nuevas aleatorias sobre un arbol de n claves).
//   delete  : delete vs n (claves existentes aleatorias).
//   skew    : search sobre distribuciones no uniformes (geometrica, poisson,
//             binomial, binomial negativa). Claves frecuentes y poco consultadas,
//             siempre claves distintas en consultas consecutivas.
//   repeat  : tiempo del k-esimo acceso consecutivo a una misma clave poco buscada.
//   control : misma clave repetida vs claves distintas (verifica que repetir
//             la misma clave reduce artificialmente el tiempo).
//
// Opciones (todas con --nombre valor):
//   --runs 32        repeticiones por punto (>= 32 para el informe)
//   --lower 1024 --upper 1048576 --step 2   rango geometrico de n (search/insert/delete)
//   --n 100000       tamano fijo para skew/repeat/control
//   --ops 100000     consultas por lote (search/skew/control)
//   --mut-ops 0      operaciones por lote en insert/delete (0 = auto: clamp(n/10,100,100000))
//   --cold-ops 10000 consultas a claves poco frecuentes por lote (skew)
//   --hot 1000       rango medio de las claves frecuentes (n debe ser >= 20*hot)
//   --dist geometric,poisson,binomial,negbinomial
//   --keys 50        claves frias distintas por repeticion (repeat)
//   --repeats 16     accesos consecutivos a cada clave fria (repeat)
//   --seed 1
//
// Unidad de todos los tiempos: nanosegundos por operacion. Cada repeticion es un
// lote de operaciones cronometrado de una sola vez (se divide por el tamano del
// lote), de modo que el costo del reloj (~20 ns) no contamina la medicion.
// t_mean y t_stdev se calculan sobre las repeticiones.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "../include/avlTree.hpp"
#include "../include/redBlackTree.hpp"
#include "../include/splayTree.hpp"
#include "../include/uhr_utils.hpp" // quartiles()

using Clock = std::chrono::steady_clock;

static volatile std::size_t g_sink = 0; // evita que el compilador elimine las consultas

[[noreturn]] static void die(const std::string& msg) {
    std::cerr << "\nError: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

static inline double ns_between(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::nano>(b - a).count();
}

// ============================================================================
// Configuracion
// ============================================================================
struct Config {
    std::string exp, out;
    std::size_t runs = 32;
    std::size_t lower = 1024, upper = 1u << 20, step = 2;
    std::size_t n = 100000;
    std::size_t ops = 100000;
    std::size_t mut_ops = 0;
    std::size_t cold_ops = 10000;
    std::size_t hot = 1000;
    std::size_t keys_per_rep = 50;
    std::size_t repeats = 16;
    std::uint64_t seed = 1;
    std::vector<std::string> dists = {"zipf","geometric", "poisson", "binomial", "negbinomial"};
};

static std::uint64_t mix(std::uint64_t a, std::uint64_t b) { // splitmix64
    std::uint64_t z = a + 0x9e3779b97f4a7c15ULL * (b + 1);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// ============================================================================
// Salida CSV y acumulacion de muestras
// ============================================================================
class CsvWriter {
    std::ofstream f;
public:
    explicit CsvWriter(const std::string& path) : f(path) {
        if (!f.is_open()) die("no se pudo abrir " + path);
        f << "exp,tree,variant,x,runs,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";
    }
    void row(const std::string& exp, const std::string& tree, const std::string& variant,
             double x, std::vector<double> v) {
        const double m = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
        double ss = 0;
        for (double t : v) ss += (t - m) * (t - m);
        const double sd = std::sqrt(ss / (v.size() > 1 ? v.size() - 1 : 1));
        std::vector<double> q;
        quartiles(v, q);
        f << exp << ',' << tree << ',' << variant << ',' << x << ',' << v.size() << ','
          << m << ',' << sd << ',' << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3]
          << ',' << q[4] << '\n';
        f.flush();
    }
};

// Acumula una muestra por repeticion para cada (arbol, variante, x).
class Collector {
    std::map<std::tuple<std::string, std::string, double>, std::vector<double>> m;
public:
    void add(const std::string& tree, const std::string& variant, double x, double value) {
        m[std::make_tuple(tree, variant, x)].push_back(value);
    }
    void flush(CsvWriter& w, const std::string& exp) {
        for (auto& kv : m)
            w.row(exp, std::get<0>(kv.first), std::get<1>(kv.first), std::get<2>(kv.first), kv.second);
        m.clear();
    }
};

// ============================================================================
// Despacho sobre los tres arboles
// ============================================================================
template <class T> struct Tag { using type = T; };

template <class F> void for_each_tree(F&& f) {
    f(Tag<AVLTree<int>>{}, "avl");
    f(Tag<RedBlackTree<int>>{}, "rb");
    f(Tag<SplayTree<int>>{}, "splay");
}

// ============================================================================
// Datos
// ============================================================================
// total claves distintas (permutacion aleatoria de 1..total): el orden es el de insercion.
static std::vector<int> make_keys(std::size_t total, std::uint64_t seed) {
    std::vector<int> k(total);
    std::iota(k.begin(), k.end(), 1);
    std::mt19937_64 g(seed);
    std::shuffle(k.begin(), k.end(), g);
    return k;
}

template <class Tree>
static void build(Tree& t, const std::vector<int>& keys, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) t.insert(keys[i]);
}

// Muestreador de "rangos de popularidad" segun una distribucion discreta no uniforme.
// Se re-muestrea si el valor cae fuera de [0, n).
class RankSampler {
    std::function<long long(std::mt19937_64&)> draw;
    std::size_t n;
public:
    RankSampler(const std::string& name, std::size_t n_, std::size_t hot) : n(n_) {
        if (name == "zipf") {
            // Genera pesos no normalizados para Zipf (s = 1.0)
            // Se usa k + 1 porque los rangos en el vector van de 0 a n-1
            const double s = 1.0; 
            std::vector<double> weights(n_);
            for (std::size_t i = 0; i < n_; ++i) {
                weights[i] = 1.0 / std::pow(static_cast<double>(i + 1), s);
            }
            std::discrete_distribution<long long> d(weights.begin(), weights.end());
            draw = [d](std::mt19937_64& g) mutable { return d(g); };
        } else if (name == "geometric") {
            std::geometric_distribution<long long> d(1.0 / hot);
            draw = [d](std::mt19937_64& g) mutable { return d(g); };
        } else if (name == "poisson") {
            std::poisson_distribution<long long> d(static_cast<double>(hot));
            draw = [d](std::mt19937_64& g) mutable { return d(g); };
        } else if (name == "binomial") {
            std::binomial_distribution<long long> d(static_cast<long long>(2 * hot), 0.5);
            draw = [d](std::mt19937_64& g) mutable { return d(g); };
        } else if (name == "negbinomial") {
            const long long k = 10;
            std::negative_binomial_distribution<long long> d(k, static_cast<double>(k) / (k + hot));
            draw = [d](std::mt19937_64& g) mutable { return d(g); };
        } else {
            die("distribucion desconocida: " + name +
                " (validas: zipf, geometric, poisson, binomial, negbinomial)");
        }
    }

    std::size_t operator()(std::mt19937_64& g) {
        for (;;) {
            long long r = draw(g);
            if (r >= 0 && static_cast<std::size_t>(r) < n) return static_cast<std::size_t>(r);
        }
    }
};

// Consultas frecuentes: rango ~ distribucion, clave = by_pop[rango]. Consecutivas distintas.
static std::vector<int> gen_hot(const std::vector<int>& by_pop, RankSampler& s,
                                std::size_t count, std::mt19937_64& g) {
    std::vector<int> q;
    q.reserve(count);
    int prev = -1;
    while (q.size() < count) {
        int key = by_pop[s(g)];
        if (key == prev) continue;
        q.push_back(key);
        prev = key;
    }
    return q;
}

// Consultas poco frecuentes: uniformes sobre los rangos [n/2, n), donde la
// distribucion tiene masa ~0 (por eso se exige n >= 20*hot). Consecutivas distintas.
static std::vector<int> gen_cold(const std::vector<int>& by_pop, std::size_t count,
                                 std::mt19937_64& g) {
    const std::size_t n = by_pop.size();
    std::uniform_int_distribution<std::size_t> U(n / 2, n - 1);
    std::vector<int> q;
    q.reserve(count);
    int prev = -1;
    while (q.size() < count) {
        int key = by_pop[U(g)];
        if (key == prev) continue;
        q.push_back(key);
        prev = key;
    }
    return q;
}

// ============================================================================
// Mediciones por lote (ns/operacion). Incluyen comprobacion de correccion
// fuera del cronometro.
// ============================================================================
template <class Tree>
static double time_search_batch(Tree& t, const std::vector<int>& q) {
    std::size_t found = 0;
    auto a = Clock::now();
    for (int k : q) found += t.search(k);
    auto b = Clock::now();
    if (found != q.size()) die("search no encontro una clave que deberia existir");
    g_sink = g_sink + found;
    return ns_between(a, b) / q.size();
}

template <class Tree>
static double time_insert_batch(Tree& t, const int* keys, std::size_t cnt) {
    auto a = Clock::now();
    for (std::size_t i = 0; i < cnt; ++i) t.insert(keys[i]);
    auto b = Clock::now();
    return ns_between(a, b) / cnt;
}

template <class Tree>
static double time_remove_batch(Tree& t, const int* keys, std::size_t cnt) {
    auto a = Clock::now();
    for (std::size_t i = 0; i < cnt; ++i) t.remove(keys[i]);
    auto b = Clock::now();
    return ns_between(a, b) / cnt;
}

// Costo de una pareja now()-now() (mediana), para restar en mediciones unitarias.
static double clock_overhead_ns() {
    std::vector<double> e(200000);
    for (auto& x : e) {
        auto a = Clock::now();
        auto b = Clock::now();
        x = ns_between(a, b);
    }
    std::nth_element(e.begin(), e.begin() + e.size() / 2, e.end());
    return e[e.size() / 2];
}

static void progress(const std::string& tag, const std::string& what, std::size_t r, std::size_t R) {
    std::cerr << "\r[" << tag << "] " << what << "  rep " << (r + 1) << "/" << R << "      " << std::flush;
}

static std::size_t mut_batch(const Config& c, std::size_t n) {
    if (c.mut_ops) return c.mut_ops;
    return std::max<std::size_t>(100, std::min<std::size_t>(n / 10, 100000));
}

// ============================================================================
// EXPERIMENTO 1: search vs n (uniforme)
// ============================================================================
static void exp_search(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("search", "n=" + std::to_string(n), r, c.runs);
            std::uint64_t s = mix(mix(mix(c.seed, 1), n), r);
            auto keys = make_keys(n, s);
            std::mt19937_64 g(mix(s, 7));
            std::uniform_int_distribution<std::size_t> U(0, n - 1);
            auto gen_q = [&](std::size_t cnt) {
                std::vector<int> q(cnt);
                for (auto& x : q) x = keys[U(g)];
                return q;
            };
            auto warm = gen_q(c.ops), meas = gen_q(c.ops);

            for_each_tree([&](auto tag, const char* name) {
                using Tree = typename decltype(tag)::type;
                Tree t;
                build(t, keys, n);
                time_search_batch(t, warm); // estado estacionario (relevante para splay)
                col.add(name, "uniform", static_cast<double>(n), time_search_batch(t, meas));
            });
        }
        col.flush(w, "search");
    }
    std::cerr << std::endl;
}

// ============================================================================
// EXPERIMENTO 2: insert vs n
// ============================================================================
static void exp_insert(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        const std::size_t m = mut_batch(c, n);
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("insert", "n=" + std::to_string(n) + " lote=" + std::to_string(m), r, c.runs);
            std::uint64_t s = mix(mix(mix(c.seed, 2), n), r);
            auto keys = make_keys(n + m, s); // [0,n): base, [n,n+m): claves nuevas

            for_each_tree([&](auto tag, const char* name) {
                using Tree = typename decltype(tag)::type;
                Tree t;
                build(t, keys, n);
                double v = time_insert_batch(t, keys.data() + n, m);
                if (t.size() != n + m) die("insert: tamano final inesperado");
                col.add(name, "insert_random", static_cast<double>(n), v);
            });
        }
        col.flush(w, "insert");
    }
    std::cerr << std::endl;
}

// ============================================================================
// EXPERIMENTO 3: delete vs n
// ============================================================================
static void exp_delete(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        const std::size_t m = std::min(mut_batch(c, n), n);
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("delete", "n=" + std::to_string(n) + " lote=" + std::to_string(m), r, c.runs);
            std::uint64_t s = mix(mix(mix(c.seed, 3), n), r);
            auto keys = make_keys(n, s);
            auto victims = keys; // m claves existentes, en orden aleatorio distinto al de insercion
            std::mt19937_64 g(mix(s, 7));
            std::shuffle(victims.begin(), victims.end(), g);

            for_each_tree([&](auto tag, const char* name) {
                using Tree = typename decltype(tag)::type;
                Tree t;
                build(t, keys, n);
                double v = time_remove_batch(t, victims.data(), m);
                if (t.size() != n - m) die("delete: tamano final inesperado (remove fallo?)");
                col.add(name, "delete_random", static_cast<double>(n), v);
            });
        }
        col.flush(w, "delete");
    }
    std::cerr << std::endl;
}

// ============================================================================
// EXPERIMENTO 4: distribuciones sesgadas, claves frecuentes vs poco frecuentes
// ============================================================================
static void check_skew_params(const Config& c) {
    if (c.n < 20 * c.hot)
        die("skew/repeat/control requieren n >= 20*hot (n=" + std::to_string(c.n) +
            ", hot=" + std::to_string(c.hot) + ")");
}

static void exp_skew(const Config& c, CsvWriter& w) {
    check_skew_params(c);
    for (const std::string& dist : c.dists) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("skew", dist, r, c.runs);
            std::uint64_t s = mix(mix(mix(c.seed, 4), std::hash<std::string>{}(dist)), r);
            auto keys = make_keys(c.n, s);
            // La popularidad NO debe correlacionar con el orden de insercion: permutacion independiente.
            auto by_pop = keys;
            std::mt19937_64 g(mix(s, 7));
            std::shuffle(by_pop.begin(), by_pop.end(), g);

            RankSampler sampler(dist, c.n, c.hot);
            auto warm = gen_hot(by_pop, sampler, c.ops, g);
            auto hot = gen_hot(by_pop, sampler, c.ops, g);
            auto cold = gen_cold(by_pop, c.cold_ops, g);

            for_each_tree([&](auto tag, const char* name) {
                using Tree = typename decltype(tag)::type;
                Tree t;
                build(t, keys, c.n);
                time_search_batch(t, warm); // el splay se adapta a la distribucion
                col.add(name, dist + ":hot", static_cast<double>(c.n), time_search_batch(t, hot));
                // El lote frio se mide a continuacion, sobre el arbol ya adaptado.
                col.add(name, dist + ":cold", static_cast<double>(c.n), time_search_batch(t, cold));
            });
        }
        col.flush(w, "skew");
    }
    std::cerr << std::endl;
}

// ============================================================================
// EXPERIMENTO 5: k-esimo acceso consecutivo a una clave poco buscada
// ============================================================================
static void exp_repeat(const Config& c, CsvWriter& w) {
    check_skew_params(c);
    const std::string dist = c.dists.front();
    const double overhead = clock_overhead_ns();
    std::cerr << "[repeat] sobrecosto del reloj restado: " << overhead << " ns\n";
    Collector col;
    for (std::size_t r = 0; r < c.runs; ++r) {
        progress("repeat", dist, r, c.runs);
        std::uint64_t s = mix(mix(c.seed, 5), r);
        auto keys = make_keys(c.n, s);
        auto by_pop = keys;
        std::mt19937_64 g(mix(s, 7));
        std::shuffle(by_pop.begin(), by_pop.end(), g);
        RankSampler sampler(dist, c.n, c.hot);
        auto warm = gen_hot(by_pop, sampler, c.ops, g);

        // claves frias distintas (por repeticion)
        std::vector<std::size_t> ranks(c.n - c.n / 2);
        std::iota(ranks.begin(), ranks.end(), c.n / 2);
        std::shuffle(ranks.begin(), ranks.end(), g);
        const std::size_t K = std::min(c.keys_per_rep, ranks.size());

        for_each_tree([&](auto tag, const char* name) {
            using Tree = typename decltype(tag)::type;
            Tree t;
            build(t, keys, c.n);
            time_search_batch(t, warm);
            std::vector<double> sum(c.repeats, 0.0);
            for (std::size_t i = 0; i < K; ++i) {
                const int key = by_pop[ranks[i]];
                for (std::size_t j = 0; j < c.repeats; ++j) {
                    auto a = Clock::now();
                    bool f = t.search(key);
                    auto b = Clock::now();
                    if (!f) die("repeat: clave inexistente");
                    g_sink = g_sink + f;
                    sum[j] += std::max(0.0, ns_between(a, b) - overhead);
                }
            }
            for (std::size_t j = 0; j < c.repeats; ++j)
                col.add(name, "cold_repeat", static_cast<double>(j + 1), sum[j] / K);
        });
    }
    col.flush(w, "repeat");
    std::cerr << std::endl;
}

// ============================================================================
// EXPERIMENTO 6: control. Misma clave repetida vs claves distintas
// ============================================================================
static void exp_control(const Config& c, CsvWriter& w) {
    check_skew_params(c);
    const std::string dist = c.dists.front();
    Collector col;
    for (std::size_t r = 0; r < c.runs; ++r) {
        progress("control", dist, r, c.runs);
        std::uint64_t s = mix(mix(c.seed, 6), r);
        auto keys = make_keys(c.n, s);
        auto by_pop = keys;
        std::mt19937_64 g(mix(s, 7));
        std::shuffle(by_pop.begin(), by_pop.end(), g);
        RankSampler sampler(dist, c.n, c.hot);
        auto warm = gen_hot(by_pop, sampler, c.ops, g);
        auto hot = gen_hot(by_pop, sampler, c.ops, g);
        auto cold = gen_cold(by_pop, c.ops, g);
        std::vector<int> same(c.ops, by_pop[c.n - 1]); // clave de menor popularidad, repetida

        for_each_tree([&](auto tag, const char* name) {
            using Tree = typename decltype(tag)::type;
            Tree t;
            build(t, keys, c.n);
            time_search_batch(t, warm);
            col.add(name, "distinct_hot", static_cast<double>(c.n), time_search_batch(t, hot));
            col.add(name, "distinct_cold", static_cast<double>(c.n), time_search_batch(t, cold));
            col.add(name, "same_key_cold", static_cast<double>(c.n), time_search_batch(t, same));
        });
    }
    col.flush(w, "control");
    std::cerr << std::endl;
}

// ============================================================================
// main
// ============================================================================
static void usage(const char* p) {
    std::cerr << "Uso: " << p << " <search|insert|delete|skew|repeat|control|all> <salida.csv|prefijo> [opciones]\n"
              << "Ver la cabecera de bench_trees.cpp para la lista de opciones.\n";
    std::exit(EXIT_FAILURE);
}

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> r;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) if (!tok.empty()) r.push_back(tok);
    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 3) usage(argv[0]);
    Config c;
    c.exp = argv[1];
    c.out = argv[2];

    for (int i = 3; i < argc; i += 2) {
        if (i + 1 >= argc) usage(argv[0]);
        std::string k = argv[i], v = argv[i + 1];
        try {
            if (k == "--runs") c.runs = std::stoull(v);
            else if (k == "--lower") c.lower = std::stoull(v);
            else if (k == "--upper") c.upper = std::stoull(v);
            else if (k == "--step") c.step = std::stoull(v);
            else if (k == "--n") c.n = std::stoull(v);
            else if (k == "--ops") c.ops = std::stoull(v);
            else if (k == "--mut-ops") c.mut_ops = std::stoull(v);
            else if (k == "--cold-ops") c.cold_ops = std::stoull(v);
            else if (k == "--hot") c.hot = std::stoull(v);
            else if (k == "--keys") c.keys_per_rep = std::stoull(v);
            else if (k == "--repeats") c.repeats = std::stoull(v);
            else if (k == "--seed") c.seed = std::stoull(v);
            else if (k == "--dist") c.dists = split(v);
            else die("opcion desconocida: " + k);
        } catch (const std::exception&) {
            die("valor invalido para " + k + ": " + v);
        }
    }

    if (c.runs < 4) die("--runs debe ser >= 4 (el enunciado pide >= 32)");
    if (c.runs < 32) std::cerr << "Aviso: --runs < 32, el boletin pide al menos 32 repeticiones.\n";
    if (c.step < 2 || c.lower == 0 || c.lower > c.upper) die("rango de n invalido (--step >= 2, 0 < lower <= upper)");
    if (c.dists.empty() || c.ops == 0 || c.cold_ops == 0 || c.hot == 0 || c.repeats == 0 || c.keys_per_rep == 0)
        die("parametros invalidos");

    const std::vector<std::string> all = {"search", "insert", "delete", "skew", "repeat", "control"};
    std::vector<std::string> todo;
    if (c.exp == "all") todo = all;
    else if (std::find(all.begin(), all.end(), c.exp) != all.end()) todo = {c.exp};
    else usage(argv[0]);

    for (const std::string& e : todo) {
        std::string path = (c.exp == "all") ? c.out + "_" + e + ".csv" : c.out;
        CsvWriter w(path);
        if (e == "search") exp_search(c, w);
        else if (e == "insert") exp_insert(c, w);
        else if (e == "delete") exp_delete(c, w);
        else if (e == "skew") exp_skew(c, w);
        else if (e == "repeat") exp_repeat(c, w);
        else if (e == "control") exp_control(c, w);
        std::cerr << "[" << e << "] listo -> " << path << std::endl;
    }
    return 0;
}