// ______________________________________________________________________
//
// Wave equation solver using finite differences
// 
// ______________________________________________________________________

#include <cmath>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <csignal>
#include <chrono>

// __________________________________________________________________________
//
// Function headers
// __________________________________________________________________________

void write_grid(const std::vector<double>& U, 
                unsigned int Nx, 
                unsigned int Ny, 
                double dx,
                double dy,
                unsigned int it,
                unsigned int padding);

// __________________________________________________________________________
//
// Main function
// __________________________________________________________________________

int main(int argc, char* argv[]) {

    // _______________________________________________________________________
    // Simulation parameters
        
    // Number of points of the grid in the x and y directions
    unsigned int Nx         = 16;
    unsigned int Ny         = 32;

    // Grid size
    double Lx               = 5.;
    double Ly               = 5.;

    // Time
    double simulation_time  = 5.;
    double CFL_factor       = 0.5;

    // Sound speed
    double c                = 1.;

    // Outputs
    int print_period        = 100;
    int output_period       = 100;

    // Boundary conditions
    int boundary_conditions = 1; // 0: Dirichlet, 1: Neumann

    // Initial Gaussian parameters
    double A                = 1;
    double xp              = 0.5;
    double yp              = 0.5;
    double fwhm             = 0.1;

    // _______________________________________________________________________
    // Parse command line arguments

    std::string help = "Usage: " + std::string(argv[0]) + " [options]\n"
                       "Options:\n"
                       "  --size <Nx> <Ny> : number of points in the x and y directions\n"
                       "  --domain-length <Lx> <Ly> : domain length in the x and y directions\n"
                       "  --time <simulation_time> : simulation time\n"
                       "  --cfl <CFL_factor> : CFL factor\n"
                       "  --sound-speed <c> : sound speed\n"
                       "  --print-period <print_period> : print period\n"
                       "  --output-period <output_period> : output period\n"
                       "  --boundary <boundary_conditions> : boundary conditions (0: Dirichlet, 1: Neumann)\n"
                       "  --initial-gaussian <A> <xp> <yp> <fwhm> : initial Gaussian parameters\n"
                       "  --help : show this message\n";

    if (argc > 1) {
        unsigned int iarg = 1;
        while (iarg < argc) {
            if (std::string(argv[iarg]) == "--size") {
                Nx = std::stoi(argv[iarg + 1]);
                Ny = std::stoi(argv[iarg + 2]);
                iarg += 3;
            } else if (std::string(argv[iarg]) == "--domain-length") {
                Lx = std::stod(argv[iarg + 1]);
                Ly = std::stod(argv[iarg + 2]);
                iarg += 3;
            } else if (std::string(argv[iarg]) == "--time") {
                simulation_time = std::stod(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--cfl") {
                CFL_factor = std::stod(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--sound-speed") {
                c = std::stod(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--print-period") {
                print_period = std::stoi(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--output-period") {
                output_period = std::stoi(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--boundary") {
                boundary_conditions = std::stoi(argv[iarg + 1]);
                iarg += 2;
            } else if (std::string(argv[iarg]) == "--initial-gaussian") {
                A = std::stod(argv[iarg + 1]);
                xp = std::stod(argv[iarg + 2]);
                yp = std::stod(argv[iarg + 3]);
                fwhm = std::stod(argv[iarg + 4]);
                iarg += 5;
            } else if (std::string(argv[iarg]) == "--help" || std::string(argv[iarg]) == "-h") {
                std::cout << help;
                std::exit(0);
            } else {
                std::cerr << " Argument " << argv[iarg] << " is not recognized." << std::endl;
                std::cerr << help;
                std::raise(SIGABRT);
            }
        }
    }


    // __________________________________________________________________________
    // Initialization of parameters

    // Space step
    const double dx = Lx / (Nx - 1);
    const double dy = Ly / (Ny - 1);

    // Time step
    const double dt = CFL_factor * std::min(dx,dy) / c;

    // coefficients
    const double cx2 = (c * dt / dx) * (c * dt / dx);
    const double cy2 = (c * dt / dy) * (c * dt / dy);

    // Number of time steps
    const unsigned int number_of_iteration = static_cast<int>(std::floor(simulation_time / dt));

    // Gaussian parameters
    xp = Lx * xp;
    yp = Ly * yp;
    fwhm = sqrt(Lx*Lx + Ly*Ly) * fwhm;

    // For diags, create a folder `diags` if it does not exist
    system("rm -rf diags");
    system("mkdir -p diags");

    // Maximum number of characters for the iteration number
    const unsigned int iteration_number_padding =
        number_of_iteration == 0
            ? 1
            : static_cast<unsigned int>(std::log10(number_of_iteration)) + 1;

    // __________________________________________________________________________
    // Print summary of parameters

    std::stringstream summary("");

    summary << " _____________________________________________\n\n";
    summary << " Wave equation solver using finite differences\n";
    summary << " _____________________________________________\n";
    summary << "\n";
    summary << " Parameters:\n";
    summary << "  - Number of points in the x and y direction: " << Nx << " " << Ny << "\n";
    summary << "  - Grid size in the x direction: " << Lx << " " << Ly << "\n";
    summary << "  - Space step in the x direction: " << dx << " " << dy << "\n";
    if (boundary_conditions == 0) {
        summary << "  - Boundary conditions: Dirichlet\n";
    } else {
        summary << "  - Boundary conditions: Neumann\n";
    }
    summary << "  - Simulation time: " << simulation_time << "\n";
    summary << "  - Time step: " << dt << "\n";
    summary << "  - Number of time steps: " << number_of_iteration << "\n";
    summary << "  - CFL factor: " << CFL_factor << "\n";
    summary << "  - Sound speed: " << c << "\n";
    summary << "  - Grid output period: " << output_period << "\n";
    summary << "  - Total memory used for grids: " << Nx*Ny*3.*sizeof(double)/(1024.*1024) << " MB\n";
    summary << std::endl;

    std::cout << summary.str();

    // __________________________________________________________________________
    // Initialize the grids using a Gaussian perturbation

    std::vector<double> U(Nx * Ny, 0.0);
    std::vector<double> U_next(Nx * Ny, 0.0);
    std::vector<double> U_prev(Nx * Ny, 0.0);

    // Compute initial condition for U_prev
    for (unsigned int i = 1; i < Nx - 1; i++) {
        const double xsquare = (i * dx - xp) * (i * dx - xp);
        for (unsigned int j = 1; j < Ny - 1; j++) {
            const double ysquare = (j * dy - yp) * (j * dy - yp);
            const double rsquare = xsquare + ysquare;
            U_prev[i*Ny + j] = A*std::exp(-rsquare / (0.5*fwhm*fwhm));
        }
    }

    // Compute initial condition for U
    for (unsigned int i = 1; i < Nx -1; i++) {  
        for (unsigned int j = 1; j < Ny -1; j++) {

            const int index = i*Ny + j;

            U[index] = U_prev[index]
            + 0.5 * cx2 * (U_prev[index + Ny] - 2*U_prev[index] + U_prev[index - Ny])
            + 0.5 * cy2 * (U_prev[i*Ny + j+1] - 2*U_prev[i*Ny + j] + U_prev[i*Ny + j-1]);
        }
    } 

    write_grid(U, Nx, Ny, dx,dy, 0, iteration_number_padding);


    // __________________________________________________________________________
    // Time loop

    std::cout << " ----------------------------------------- " << std::endl;
    std::cout << " MAIN LOOP                               | " << std::endl;
    std::cout << " ----------------------------------------- " << std::endl;
    std::cout << " Iteration |      Sum |                  | " << std::endl;
    std::cout << " ----------------------------------------- " << std::endl;

    auto time_start = std::chrono::high_resolution_clock::now();

    for (unsigned int it = 1; it < number_of_iteration ; it++) {

        // Sum of the grid
        double sum = 0;

        // Update the grid
        for (unsigned int i = 1; i < Nx - 1; i++) {
            for (unsigned int j = 1; j < Ny - 1; j++) {

                const int index = i*Ny + j;

                U_next[index] = 2*U[index] - U_prev[index] 
                + cx2 * (U[index + Ny] - 2*U[index] + U[index - Ny]) 
                + cy2 * (U[index + 1] - 2*U[index] + U[index-1]);
            }
        }
            
        // Neumann Boundary conditions
        if (boundary_conditions == 1) {
            // x = 0 and x = Lx
            for (unsigned int j = 0; j < Ny; j++) {
                U_next[j] = U_next[Ny + j];
                U_next[(Nx-1)*Ny + j] = U_next[(Nx-2)*Ny + j];
            }

            // y = 0 and y = Ly
            for (unsigned int i = 0; i < Nx; i++) {
                U_next[i*Ny] = U_next[i*Ny + 1];
                U_next[i*Ny + Ny - 1] = U_next[i*Ny + Ny - 2];
            }
        } // end boundary_conditions

        // Permute the grids
        for (unsigned int i = 0; i < Nx; i++) {
            for (unsigned int j = 0; j < Ny; j++) {
                U_prev[i*Ny + j] = U[i*Ny + j];
                U[i*Ny + j] = U_next[i*Ny + j];
            }
        }

        // Compute the sum of the grid
        for (unsigned int i = 0; i < Nx; i++) {
            for (unsigned int j = 0; j < Ny; j++) {
                sum += U[i*Ny + j]*U[i*Ny + j];
            }
        }

        // write the grid to a file
        if (it % output_period == 0) {
            write_grid(U, Nx, Ny, dx,dy, it, iteration_number_padding);
        }

        // Print iteration information in the terminal
        if (it % print_period == 0) {
            std::cout << std::setw(10) << it << " | " 
                    << std::scientific << std::setprecision(2) << sum << " | "
                    << "                 | "
                    << std::endl;
        }
    }

    auto time_end = std::chrono::high_resolution_clock::now();

    std::cout << " ----------------------------------------- " << std::endl;
    std::cout << " TIMER                                   | " << std::endl;
    std::cout << " ----------------------------------------- " << std::endl;

    std::chrono::duration<double> elapsed_seconds = time_end - time_start;

    std::cout << " Elapsed time: " << elapsed_seconds.count() << " s" << std::endl;

}

// __________________________________________________________________________
//
// Function definitions
// __________________________________________________________________________

// __________________________________________________________________________
//! \brief Write the grid to a file
/*  Write the grid to a file using a json format
    Format :
    {
        "parameters" : {
            "Nx" : Nx,
            "Ny" : Ny,
            "dx" : dx,
            "dy" : dy,
            "it" : it
        },
        "data" : U
    }
*/
void write_grid(const std::vector<double>& U, 
                unsigned int Nx, 
                unsigned int Ny, 
                double dx,
                double dy,
                unsigned int it,
                unsigned int padding) {

    // File name of the form grid_0000.json
    std::stringstream filename("");
    filename << "diags/grid_" << std::setfill('0') << std::setw(padding) << it << ".json";

    std::ofstream file(filename.str());
    file <<  "{\n";
    file <<  "    \"parameters\" : {\n";
    file <<  "        \"Nx\" : " << Nx << ",\n";
    file <<  "        \"Ny\" : " << Ny << ",\n";
    file <<  "        \"dx\" : " << dx << ",\n";
    file <<  "        \"dy\" : " << dy << ",\n";
    file <<  "        \"it\" : " << it << "\n";
    file <<  "    },\n";
    file <<  "    \"data\" : [";
    for (unsigned int i = 0; i < Nx; i++) {
        for (unsigned int j = 0; j < Ny; j++) {
            file << U[i*Ny + j];
            if ((i != Nx - 1) || (j != Ny - 1)) {
                file << ", ";
            }
        }
    }
    file <<  "]\n";
    file <<  "}\n";

    file.close();
}
