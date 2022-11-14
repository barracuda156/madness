#ifndef SRC_APPS_molresponse_GLOBAL_FUNCTIONS_H_
#define SRC_APPS_molresponse_GLOBAL_FUNCTIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "response_parameters.h"

struct CalcParams {
    GroundStateCalculation ground_calculation;
    Molecule molecule;
    ResponseParameters response_parameters;
};
void print_molecule(World &world, const GroundStateCalculation &g_params);
class plotCoords {
public:
    coord_3d lo, hi;

    plotCoords() {
        lo[0] = 0.0;
        lo[1] = 0.0;
        lo[2] = 0.0;
        hi[0] = 0.0;
        hi[1] = 0.0;
        hi[2] = 0.0;
    }
    plotCoords(size_t direction, double Lp) {
        lo[0] = 0.0;
        lo[1] = 0.0;
        lo[2] = 0.0;

        hi[0] = 0.0;
        hi[1] = 0.0;
        hi[2] = 0.0;

        lo[direction] = -Lp;
        hi[direction] = Lp;
    }
};
plotCoords SetPlotCoord(size_t i, double Lp);

CalcParams initialize_calc_params(World &world, const std::string &input_file);
// kinetic energy operator on response vector
response_space T(World &world, response_space &f);

auto ground_exchange(const vecfuncT &phi0, const X_space &x, const bool compute_y) -> X_space;
auto response_exchange(const vecfuncT &phi0, const response_matrix &x, const response_matrix &x_dagger, bool static_response) -> response_matrix;
vecfuncT K(vecfuncT &ket, vecfuncT &bra, vecfuncT &vf);
vecfuncT newK(const vecfuncT &ket, const vecfuncT &bra, const vecfuncT &vf);
static double rsquared(const coord_3d &r) { return r[0] * r[0] + r[1] * r[1] + r[2] * r[2]; }
/// Mask function to switch from 0 to 1 smoothly at boundary
#endif// SRC_APPS_molresponse_GLOBAL_FUNCTIONS_H_
