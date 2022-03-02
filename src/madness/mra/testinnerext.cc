

//#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include <madness/mra/mra.h>
#include <array>
using namespace madness;

bool smalltest = true;

double ttt, sss;
#define START_TIMER world.gop.fence(); ttt=wall_time(); sss=cpu_time()
#define END_TIMER(msg) ttt=wall_time()-ttt; sss=cpu_time()-sss; if (world.rank()==0) printf("timer: %-20.20s %8.2fs %8.2fs\n", msg, sss, ttt)

typedef std::shared_ptr< FunctionFunctorInterface<double,3> > functorT;

static const double R = 1.4;    // bond length
static const double L = 5.0*R; // box size
static const long k = 6;        // wavelet order
static const double thresh = 1e-8; // precision

static double alpha_func(const coord_3d& r) {
    const double x=r[0], y=r[1], z=r[2];
    return ((x*x + y*y + z*z) * sin(x*x + y*y + z*z));
};

static double beta_func(const coord_3d& r) {
    const double x=r[0], y=r[1], z=r[2];
    return (exp(- x*x - y*y - z*z));
};

class alpha_functor : public FunctionFunctorInterface<double,3> {
private:
    double coeff;
public:
    alpha_functor(double coeff=1.0) : coeff(coeff) {}

    virtual double operator()(const coord_3d& r) const {
        const double x=r[0], y=r[1], z=r[2];
        return (coeff * (x*x + y*y + z*z) * sin(x*x + y*y + z*z));
    }
};

class beta_functor : public FunctionFunctorInterface<double,3> {
private:
    double coeff;
public:
    beta_functor(double coeff=1.0) : coeff(coeff) {}

    virtual double operator()(const coord_3d& r) const {
        return (coeff * exp(- r[0]*r[0] - r[1]*r[1] - r[2]*r[2]));
    }
};

struct gauss_1d {
    double s=1.0;
    double operator()(const double x) const {return 1.0/(s * std::pow(constants::pi,0.25))* exp(-0.5*x*x/(s*s));}
};
template<typename T, std::size_t NDIM>
struct gauss {

    T operator()(const Vector<double,NDIM>& coord) const {
        double result=1.0;
        for (std::size_t i=0; i<NDIM; ++i) result*=gauss_1d()(coord[i]);
        return result;
    }
};

bool is_like(double a, double b, double tol) {
    return (std::abs((a - b)/a) <= tol);
}

int test_partial_inner(World& world) {
    print("\ntesting partial inner\n");
    real_function_1d one_1d=real_factory_1d(world).functor([](const coord_1d& r){return 1.0;});
    real_function_2d one_2d=real_factory_2d(world).functor([](const coord_2d& r){return 1.0;});
    real_function_1d gauss_1d=real_factory_1d(world).functor(gauss<double,1>());

    real_function_2d f2=real_factory_2d(world).functor(gauss<double,2>());
    real_function_3d f3=real_factory_3d(world).functor(gauss<double,3>());
    real_function_4d f4=real_factory_4d(world).functor(gauss<double,4>());
    real_function_2d f=real_factory_2d(world).functor(gauss<double,2>());
    real_function_2d g=real_factory_2d(world).functor(gauss<double,2>());
    real_function_2d h=real_factory_2d(world).functor(gauss<double,2>());
    real_function_1d g1=real_factory_1d(world).functor(gauss<double,1>());

    double ovlp_1d=inner(gauss_1d,gauss_1d);
//    double ref=inner(gauss_1d,gauss_1d) * gauss_1d.trace() * gauss_1d.trace();
//    print("reference result",ref);

    double f2norm2_1=inner(f2,f2);
    real_function_4d b=inner(f3,f3,{0},{0});
//    print("b tree");
//    b.print_tree();
//    print("f2 tree");
//    f2.print_tree();
    double bnorm2=inner(b,b);
    double f2norm2=inner(f2,f2);
    double b_val=inner(f4,b);
    print("bnorm",bnorm2);
    print("f2norm before ",f2norm2_1);
    print("f2norm after  ",f2norm2);
    print("b",b_val);
    print("ovlp^4",std::pow(ovlp_1d,4.0));

//    print("\n\ng1 tree (inconsistent state!)");
//    g1.print_tree();


//    // will return f(z1,x2) = int f(x1,y1,z1) g(x2,x1,y1);
//    real_function_2d a=inner(f,g,{0,1},{1,2});
//    real_function_4d b=inner(f,g,{0},{1});
//    real_function_1d c=inner(h,f,{0,1},{1,2});
    return 0;
}

template<std::size_t NDIM>
void initialize(World& world) {
    FunctionDefaults<NDIM>::set_defaults(world);
    FunctionDefaults<NDIM>::set_k(k);
    FunctionDefaults<NDIM>::set_thresh(thresh);
    FunctionDefaults<NDIM>::set_refine(true);
    FunctionDefaults<NDIM>::set_initial_level(5);
    FunctionDefaults<NDIM>::set_truncate_mode(1);
    FunctionDefaults<NDIM>::set_cubic_cell(-L/2, L/2);
}
int main(int argc, char** argv) {
    initialize(argc, argv);
    World world(SafeMPI::COMM_WORLD);

    int success = 0;

    startup(world,argc,argv);
    std::cout.precision(6);

    if (getenv("MAD_SMALL_TESTS")) smalltest=true;
    for (int iarg=1; iarg<argc; iarg++) if (strcmp(argv[iarg],"--small")==0) smalltest=true;
    std::cout << "small test : " << smalltest << std::endl;

    initialize<1>(world);
    initialize<2>(world);
    initialize<3>(world);
    initialize<4>(world);
    initialize<5>(world);
    initialize<6>(world);

    test_partial_inner(world);

    if (!smalltest) {
        real_function_3d alpha1 = real_factory_3d(world).f(alpha_func);
        real_functor_3d alpha1_ffi = real_functor_3d(new alpha_functor());
        
        if (world.rank() == 0) {
            print("***************************************************************************");
            print("alpha is a highly oscillatory function (x^2 sin(x^2)).");
            print("beta is a pretty boring Gaussian.");
            print("For the first two timers, the cost of computing alpha in");
            print("the numerical basis is not included. We see that since beta");
            print("is a simple Gaussian, it is cheaper to use the inner() method.\n");
        }
        
        START_TIMER;
        real_function_3d beta = real_factory_3d(world).f(beta_func);
        double ab = alpha1.inner(beta);
        END_TIMER("1. < a | b >)");
        
        START_TIMER;
        real_functor_3d beta_ffi = real_functor_3d(new beta_functor());
        double ab_ffi = alpha1.inner_ext(beta_ffi);
        END_TIMER("3. < a | b_ffi >");
        
        if (world.rank() == 0) {
            print("\n***************************************************************************");
            print("For the next two timers, the cost of computing beta in");
            print("the numerical basis is not included. We see that since alpha");
            print("is complicated, it is cheaper to use the inner_ext() method.\n");
        }
        
        START_TIMER;
        real_function_3d alpha = real_factory_3d(world).f(alpha_func);
        double ba = beta.inner(alpha);
        END_TIMER("4. < b | a >");
        
        START_TIMER;
        real_functor_3d alpha_ffi = real_functor_3d(new alpha_functor());
        double ba_ffi = beta.inner_ext(alpha_ffi);
        END_TIMER("6. < b | a_ffi >");
        
        double aa = alpha.inner(alpha);
        double bb = beta.inner(beta);
        
        double aa_ffi = alpha.inner_ext(alpha_ffi);
        double bb_ffi = beta.inner_ext(beta_ffi);
        
        if (world.rank() == 0) {
            print("\nTest for correctness");
            print("***************************************************************************");
            printf("<a|a> (using inner() with Function) =                      %7.10f\n", aa);
            printf("<a|a> (using inner_ext() with FunctionFunctor Interface) = %7.10f\n", aa_ffi);
            print("***************************************************************************");
            printf("<b|b> (using inner() with Function) =                      %7.10f\n", bb);
            printf("<b|b> (using inner_ext() with FunctionFunctor Interface) = %7.10f\n", bb_ffi);
            print("***************************************************************************");
            printf("<a|b> (using inner() with Function) =                      %7.10f\n", ab);
            printf("<a|b> (using inner_ext() with FunctionFunctor Interface) = %7.10f\n", ab_ffi);
            printf("<b|a> (using inner() with Function) =                      %7.10f\n", ba);
            printf("<b|a> (using inner_ext() with FunctionFunctor Interface) = %7.10f\n", ba_ffi);
            print("***************************************************************************");
        }
        
        real_function_3d alphabeta = real_factory_3d(world);
        alphabeta = alpha + beta;
        double aba = alphabeta.inner(alpha);
        double aba_ffi = alphabeta.inner_ext(alpha_ffi);
        
        if (world.rank() == 0) {
            print("\nCheck that inner_ext works for Function that lacks a functor");
            print("***************************************************************************");
            printf("<a+b|a> (using inner() with Function) =                      %7.10f\n", aba);
            printf("<a+b|a> (using inner_ext() with FunctionFunctor Interface) = %7.10f\n", aba_ffi);
            print("***************************************************************************");
        }
        
        if (not is_like(aa, aa_ffi, thresh)) ++success;
        if (not is_like(bb, bb_ffi, thresh)) ++success;
        if (not is_like(ab, ab_ffi, thresh)) ++success;
        if (not is_like(ab, ba_ffi, thresh)) ++success;
    }

    world.gop.fence();

    finalize();
    return success;
}
