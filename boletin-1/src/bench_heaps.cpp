// bench_heaps.cpp
// Benchmark de Heap Binario (max-heap) y Heap Binomial (min-heap).
//
// Compilar (desde boletin-1/):
//   g++ -std=c++17 -O2 -o bench_heaps src/bench_heaps.cpp
//
// Uso:
//   ./bench_heaps <exp> <salida.csv> [opciones]
//   exp: insert | top | pop | meld | build | all
//        (con "all", <salida> es un prefijo: genera <prefijo>_<exp>.csv)
//
// Operaciones medidas (siempre sobre un heap de tamano ~n, no sobre heaps que
// crecen o se vacian por completo, para poder contrastar cada operacion con su
// complejidad teorica):
//   insert : m inserciones consecutivas sobre un heap de n elementos.   [ns/op]
//   top    : K llamadas a top() sobre un heap de n elementos.           [ns/op]
//   pop    : m extracciones consecutivas sobre un heap de n elementos.  [ns/op]
//   meld   : union de dos heaps de n elementos cada uno.                [ns/meld]
//   build  : construir un heap desde un std::vector de n elementos.    [ns/elemento]
//            (binario: make_heap, O(n); binomial: el constructor hace n insert)
//
// Todos los heaps de los experimentos insert/top/pop/meld se construyen con n
// inserciones de claves en orden aleatorio (igual para ambos heaps).
// Las claves son una permutacion aleatoria de 1..N, lo que permite verificar
// la correccion fuera del cronometro (valor esperado de top() tras cada prueba).
//
// Opciones (--nombre valor):
//   --runs 32       repeticiones por punto (>= 32 para el informe)
//   --lower 1024 --upper 1048576 --step 2   rango geometrico de n
//   --ops 100000    llamadas a top() por lote
//   --mut-ops 0     operaciones por lote en insert/pop (0 = auto: clamp(n/10,100,100000))
//   --meld-batch 0  uniones por repeticion (0 = auto: clamp(65536/n,1,64))
//   --shape worst   tamano efectivo del heap: worst (n-1, log2(n) raices binomiales),
//                   pow2 (n exacto; con n potencia de 2 el binomial es un solo arbol) o
//                   random (n + U[0,n)). Ver eff_size().
//   --seed 1
//
// CSV: n,heap,operacion,unit,runs,t_mean,t_stdev,t_Q0..t_Q4
// t_mean y t_stdev se calculan sobre las repeticiones.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "../include/binaryHeap.hpp"
#include <vector> // binomialHeap.hpp lo usa pero no lo incluye
#include "../include/binomialHeap.hpp"
#include "../include/uhr_utils.hpp" // quartiles()

using Clock = std::chrono::steady_clock;

static volatile long long g_sink = 0; // evita que el compilador elimine llamadas

[[noreturn]] static void die(const std::string& msg) {
    std::cerr << "\nError: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

static inline double ns_between(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::nano>(b - a).count();
}

// Barrera de compilador: obliga a releer memoria en cada iteracion (evita que
// top() del heap binario se "saque" fuera del bucle).
static inline void clobber() {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" ::: "memory");
#endif
}

// ============================================================================
// Configuracion
// ============================================================================
struct Config {
    std::string exp, out;
    std::size_t runs = 32;
    std::size_t lower = 1024, upper = 1u << 20, step = 2;
    std::size_t ops = 100000;
    std::size_t mut_ops = 0;
    std::size_t meld_batch = 0;
    std::string shape = "worst";
    std::uint64_t seed = 1;
};

static std::uint64_t mix(std::uint64_t a, std::uint64_t b) { // splitmix64
    std::uint64_t z = a + 0x9e3779b97f4a7c15ULL * (b + 1);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static std::size_t mut_batch(const Config& c, std::size_t n) {
    std::size_t m = c.mut_ops ? c.mut_ops
                              : std::max<std::size_t>(100, std::min<std::size_t>(n / 10, 100000));
    return std::min(m, n / 2); // nunca vaciar el heap
}

static std::size_t meld_batch(const Config& c, std::size_t n) {
    if (c.meld_batch) return c.meld_batch;
    return std::max<std::size_t>(1, std::min<std::size_t>(64, 65536 / n));
}

// Tamano efectivo del heap para un n dado. Importa por el heap binomial: su
// forma depende de la representacion binaria del tamano (un arbol por cada bit en 1).
//   worst : n-1     -> todos los bits en 1: log2(n) raices (peor caso para top y meld)
//   pow2  : n       -> un solo arbol si n es potencia de 2 (caso degenerado, top/meld ~O(1))
//   random: n + U[0,n) -> numero de raices tipico (~log2(n)/2)
static std::size_t eff_size(const Config& c, std::size_t n, std::uint64_t s) {
    if (c.shape == "pow2") return n;
    if (c.shape == "worst") return n - 1;
    std::mt19937_64 g(mix(s, 99));
    return n + static_cast<std::size_t>(g() % n);
}

// ============================================================================
// Adaptadores: misma interfaz para ambos heaps
// ============================================================================
struct BinaryAdapter {
    using H = binary_heap; // max-heap
    static constexpr const char* name = "binary";
    static void insert(H& h, int k) { h.push(k); }
    static int top(H& h) { return h.top(); }
    static void pop(H& h) { h.pop(); }
    static void meld(H& a, H& b) { a.meld(b); }
    // top() esperado para el conjunto {lo..hi}
    static int expected_top(int /*lo*/, int hi) { return hi; }
    // top() esperado tras extraer m elementos de {1..n}
    static int expected_after_pops(int n, int m) { return n - m; }
};

struct BinomialAdapter {
    using H = binomial_heap; // min-heap
    static constexpr const char* name = "binomial";
    static void insert(H& h, int k) { h.insert(k); }
    static int top(H& h) { return h.top(); }
    static void pop(H& h) { h.pop(); }
    static void meld(H& a, H& b) { a.meld(std::move(b)); }
    static int expected_top(int lo, int /*hi*/) { return lo; }
    static int expected_after_pops(int /*n*/, int m) { return m + 1; }
};

template <class T> struct Tag { using type = T; };

template <class F> void for_each_heap(F&& f) {
    f(Tag<BinaryAdapter>{});
    f(Tag<BinomialAdapter>{});
}

// ============================================================================
// CSV y acumulacion de muestras
// ============================================================================
static const char* unit_of(const std::string& op) {
    if (op == "meld") return "ns/meld";
    if (op == "build") return "ns/elem";
    return "ns/op";
}

class CsvWriter {
    std::ofstream f;
public:
    explicit CsvWriter(const std::string& path) : f(path) {
        if (!f.is_open()) die("no se pudo abrir " + path);
        f << "n,heap,operacion,unit,runs,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";
    }
    void row(double n, const std::string& heap, const std::string& op, std::vector<double> v) {
        const double m = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
        double ss = 0;
        for (double t : v) ss += (t - m) * (t - m);
        const double sd = std::sqrt(ss / (v.size() > 1 ? v.size() - 1 : 1));
        std::vector<double> q;
        quartiles(v, q);
        f << n << ',' << heap << ',' << op << ',' << unit_of(op) << ',' << v.size() << ','
          << m << ',' << sd << ',' << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3]
          << ',' << q[4] << '\n';
        f.flush();
    }
};

class Collector {
    std::map<std::tuple<std::string, std::string>, std::vector<double>> m;
public:
    void add(const std::string& heap, const std::string& op, double value) {
        m[std::make_tuple(heap, op)].push_back(value);
    }
    void flush(CsvWriter& w, double n) {
        for (auto& kv : m) w.row(n, std::get<0>(kv.first), std::get<1>(kv.first), kv.second);
        m.clear();
    }
};

// ============================================================================
// Utilidades
// ============================================================================
// Permutacion aleatoria de 1..total: el orden es el de insercion.
static std::vector<int> make_keys(std::size_t total, std::uint64_t seed) {
    std::vector<int> k(total);
    std::iota(k.begin(), k.end(), 1);
    std::mt19937_64 g(seed);
    std::shuffle(k.begin(), k.end(), g);
    return k;
}

template <class A>
static void fill(typename A::H& h, const int* keys, std::size_t cnt) {
    for (std::size_t i = 0; i < cnt; ++i) A::insert(h, keys[i]);
}

static void progress(const std::string& tag, std::size_t n, std::size_t r, std::size_t R) {
    std::cerr << "\r[" << tag << "] n=" << n << "  rep " << (r + 1) << "/" << R << "      " << std::flush;
}

static void check(bool ok, const char* what) {
    if (!ok) die(std::string("verificacion fallida: ") + what);
}

// ============================================================================
// insert: m inserciones sobre un heap de n elementos
// ============================================================================
static void exp_insert(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("insert", n, r, c.runs);
            const std::uint64_t sd = mix(mix(mix(c.seed, 1), n), r);
            const std::size_t ne = eff_size(c, n, sd), m = mut_batch(c, ne);
            auto keys = make_keys(ne + m, sd); // [0,ne) base; [ne,ne+m) nuevas
            for_each_heap([&](auto tag) {
                using A = typename decltype(tag)::type;
                typename A::H h;
                fill<A>(h, keys.data(), ne);
                auto a = Clock::now();
                fill<A>(h, keys.data() + ne, m);
                auto b = Clock::now();
                check(A::top(h) == A::expected_top(1, static_cast<int>(ne + m)), "insert");
                col.add(A::name, "insert", ns_between(a, b) / m);
            });
        }
        col.flush(w, static_cast<double>(n));
    }
    std::cerr << std::endl;
}

// ============================================================================
// top: K llamadas sobre un heap de n elementos
// ============================================================================
static void exp_top(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("top", n, r, c.runs);
            const std::uint64_t sd = mix(mix(mix(c.seed, 2), n), r);
            const std::size_t ne = eff_size(c, n, sd);
            auto keys = make_keys(ne, sd);
            for_each_heap([&](auto tag) {
                using A = typename decltype(tag)::type;
                typename A::H h;
                fill<A>(h, keys.data(), ne);
                long long sum = 0;
                auto a = Clock::now();
                for (std::size_t i = 0; i < c.ops; ++i) {
                    sum += A::top(h);
                    clobber();
                }
                auto b = Clock::now();
                check(sum == static_cast<long long>(c.ops) * A::expected_top(1, static_cast<int>(ne)), "top");
                g_sink = g_sink + sum;
                col.add(A::name, "top", ns_between(a, b) / c.ops);
            });
        }
        col.flush(w, static_cast<double>(n));
    }
    std::cerr << std::endl;
}

// ============================================================================
// pop: m extracciones sobre un heap de n elementos
// ============================================================================
static void exp_pop(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("pop", n, r, c.runs);
            const std::uint64_t sd = mix(mix(mix(c.seed, 3), n), r);
            const std::size_t ne = eff_size(c, n, sd), m = mut_batch(c, ne);
            auto keys = make_keys(ne, sd);
            for_each_heap([&](auto tag) {
                using A = typename decltype(tag)::type;
                typename A::H h;
                fill<A>(h, keys.data(), ne);
                auto a = Clock::now();
                for (std::size_t i = 0; i < m; ++i) A::pop(h);
                auto b = Clock::now();
                check(A::top(h) == A::expected_after_pops(static_cast<int>(ne), static_cast<int>(m)), "pop");
                col.add(A::name, "pop", ns_between(a, b) / m);
            });
        }
        col.flush(w, static_cast<double>(n));
    }
    std::cerr << std::endl;
}

// ============================================================================
// meld: union de dos heaps de n elementos
// ============================================================================
static void exp_meld(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        const std::size_t B = meld_batch(c, n);
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("meld", n, r, c.runs);
            const std::uint64_t sd = mix(mix(mix(c.seed, 4), n), r);
            const std::size_t ne = eff_size(c, n, sd);
            auto keys = make_keys(2 * ne, sd); // A: [0,ne)  B: [ne,2ne)
            for_each_heap([&](auto tag) {
                using A = typename decltype(tag)::type;
                using H = typename A::H;
                // B pares de heaps (unique_ptr: los heaps no deben copiarse)
                std::vector<std::unique_ptr<H>> ha, hb;
                for (std::size_t i = 0; i < B; ++i) {
                    ha.emplace_back(new H);
                    hb.emplace_back(new H);
                    fill<A>(*ha[i], keys.data(), ne);
                    fill<A>(*hb[i], keys.data() + ne, ne);
                }
                auto a = Clock::now();
                for (std::size_t i = 0; i < B; ++i) A::meld(*ha[i], *hb[i]);
                auto b = Clock::now();
                for (std::size_t i = 0; i < B; ++i)
                    check(A::top(*ha[i]) == A::expected_top(1, static_cast<int>(2 * ne)), "meld");
                col.add(A::name, "meld", ns_between(a, b) / B);
            });
        }
        col.flush(w, static_cast<double>(n));
    }
    std::cerr << std::endl;
}

// ============================================================================
// build: construir desde un vector de n elementos
// ============================================================================
static void exp_build(const Config& c, CsvWriter& w) {
    for (std::size_t n = c.lower; n <= c.upper; n *= c.step) {
        Collector col;
        for (std::size_t r = 0; r < c.runs; ++r) {
            progress("build", n, r, c.runs);
            const std::uint64_t sd = mix(mix(mix(c.seed, 5), n), r);
            const std::size_t ne = eff_size(c, n, sd);
            auto keys = make_keys(ne, sd);
            for_each_heap([&](auto tag) {
                using A = typename decltype(tag)::type;
                auto a = Clock::now();
                typename A::H h(keys);
                auto b = Clock::now();
                check(A::top(h) == A::expected_top(1, static_cast<int>(ne)), "build");
                col.add(A::name, "build", ns_between(a, b) / ne);
            });
        }
        col.flush(w, static_cast<double>(n));
    }
    std::cerr << std::endl;
}

// ============================================================================
// main
// ============================================================================
static void usage(const char* p) {
    std::cerr << "Uso: " << p << " <insert|top|pop|meld|build|all> <salida.csv|prefijo> [opciones]\n"
              << "Ver la cabecera de bench_heaps.cpp para la lista de opciones.\n";
    std::exit(EXIT_FAILURE);
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
            else if (k == "--ops") c.ops = std::stoull(v);
            else if (k == "--mut-ops") c.mut_ops = std::stoull(v);
            else if (k == "--meld-batch") c.meld_batch = std::stoull(v);
            else if (k == "--shape") c.shape = v;
            else if (k == "--seed") c.seed = std::stoull(v);
            else die("opcion desconocida: " + k);
        } catch (const std::exception&) {
            die("valor invalido para " + k + ": " + v);
        }
    }

    if (c.runs < 4) die("--runs debe ser >= 4 (el enunciado pide >= 32)");
    if (c.runs < 32) std::cerr << "Aviso: --runs < 32, el boletin pide al menos 32 repeticiones.\n";
    if (c.step < 2 || c.lower < 16 || c.lower > c.upper)
        die("rango de n invalido (--step >= 2, 16 <= lower <= upper)");
    if (c.ops == 0) die("--ops debe ser > 0");
    if (c.shape != "worst" && c.shape != "pow2" && c.shape != "random")
        die("--shape debe ser worst, pow2 o random");

    const std::vector<std::string> all = {"insert", "top", "pop", "meld", "build"};
    std::vector<std::string> todo;
    if (c.exp == "all") todo = all;
    else if (std::find(all.begin(), all.end(), c.exp) != all.end()) todo = {c.exp};
    else usage(argv[0]);

    for (const std::string& e : todo) {
        std::string path = (c.exp == "all") ? c.out + "_" + e + ".csv" : c.out;
        CsvWriter w(path);
        if (e == "insert") exp_insert(c, w);
        else if (e == "top") exp_top(c, w);
        else if (e == "pop") exp_pop(c, w);
        else if (e == "meld") exp_meld(c, w);
        else if (e == "build") exp_build(c, w);
        std::cerr << "[" << e << "] listo -> " << path << std::endl;
    }
    return 0;
}