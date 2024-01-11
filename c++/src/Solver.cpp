#include "../headers/Solver.hpp"

/***********************************************************************************************************************
 *                                                                                                                     *
 *                                                                                                                     *
 *                                                Search engine methods                                                *
 *                                                                                                                     *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * Creates a search engine for the given problem
 * @param pb an instance of the FourVoiceTexture class representing a given problem
 * @param type the type of search engine to create (see enumeration in headers/gecode_problem.hpp)
 * @param timeout the maximum time in milliseconds the solver can run for (default value is 60000)
 * @return a search engine for the given problem
 */
Base<FourVoiceTexture> *make_solver(FourVoiceTexture *pb, int type, int timeout) {
    Search::Options opts;
    //opts.threads = 0; /// as many as available
    opts.stop = Search::Stop::time(timeout); // stop after 120 seconds

    if (type == BAB_SOLVER){
        write_to_log_file("Solver type: BAB\n", LOG_FILE);
        return new BAB<FourVoiceTexture>(pb, opts);
    }
    else{
        write_to_log_file("Solver type: DFS\n", LOG_FILE);
        return new DFS<FourVoiceTexture>(pb, opts);
    }
}

/**
 * Returns the next solution space for the problem
 * @param solver a solver for the problem
 * @return an instance of the FourVoiceTexture class representing the next solution to the problem
 */
FourVoiceTexture* get_next_solution_space(Search::Base<FourVoiceTexture>* solver){ 
    FourVoiceTexture* sol_space = solver->next();
    if (sol_space == nullptr) // handle the case of no solution or time out, necessary when sending the data to OM
        return nullptr;
    //write_to_log_file(sol_space->to_string().c_str(), LOG_FILE);
    return sol_space;
}

/**
 * Returns the best solution for the problem pb. It uses a branch and bound solver with lexico-minimization of the costs
 * @param pb an instance of a FourVoiceTexture problem
 * @param timeout the maximum time in milliseconds the solver can run for (default value is 60000)
 * @return the best solution to the problem
 */
const FourVoiceTexture *find_best_solution(FourVoiceTexture *pb, int timeout, string csvFileName, string preMessage) {
    // create a new search engine
    auto* solver = make_solver(pb, BAB_SOLVER, timeout);
    Search::Statistics bestSolStats = solver->statistics();

    string solsAndTime;

    FourVoiceTexture *bestSol; // keep a pointer to the best solution
    auto start = std::chrono::high_resolution_clock::now();     /// start time
    while(FourVoiceTexture *sol = get_next_solution_space(solver)){
        auto currTime = std::chrono::high_resolution_clock::now();     /// current time
        std::chrono::duration<double> duration = currTime - start;
        solsAndTime += "," + to_string(duration.count()) + " , " + intVarArgs_to_string(sol->get_cost_vector()) + ",";
        bestSol = sol;
        bestSolStats = solver->statistics();
    }
    auto finalTime = std::chrono::high_resolution_clock::now();     /// final time
    std::chrono::duration<double> duration = finalTime - start;

    string message = "Best solution found: \n" + bestSol->to_string() + "\n";
    //std::cout << message << std::endl;
    //write_to_log_file(message.c_str(), LOG_FILE);

    auto statsCSV = preMessage + "," + to_string(duration.count()) + ",,," +
                    statistics_to_csv_string(bestSolStats) + intVarArgs_to_string(bestSol->get_cost_vector()) +
                    ",,," + statistics_to_csv_string(solver->statistics()) + "," +
                    solsAndTime + ",";
    std::cout << statsCSV << std::endl;
    //write_to_log_file(statsCSV.c_str(), csvFileName);

    return bestSol;
}

/**
 * Returns the first maxNOfSols solutions for the problem pb using the solver solverType.
 * @param pb an instance of a FourVoiceTexture problem
 * @param solverType the type of the solver to use from solver_types
 * @param maxNOfSols the maximum number of solutions we want to find (the default value is 1000)
 * @param timeout the maximum time in milliseconds the solver can run for (default value is 60000)
 * @return the first maxNOfSols solutions to the problem
 */
vector<const FourVoiceTexture *> find_all_solutions(FourVoiceTexture *pb, int solverType, int maxNOfSols, int timeout) {
    vector<const FourVoiceTexture*> sols;
    // create the search engine
    auto* solver = make_solver(pb, solverType, timeout);
    write_to_log_file("\nSearching for solutions:\n", LOG_FILE);

    int nbSol = 0;
    while(FourVoiceTexture *sol= get_next_solution_space(solver)){
        nbSol++;
        sols.push_back(sol);
        string message = "Solution found: \nSolution" + to_string(nbSol) + ": \n" + sol->to_string() + "\n"
                + "Solution " + to_string(nbSol) + "\n";
        write_to_log_file(message.c_str(), LOG_FILE);
        std::cout << message << std::endl  << statistics_to_string(solver->statistics()) << std::endl;
        write_to_log_file(statistics_to_string(solver->statistics()).c_str(), LOG_FILE);
        if (nbSol >= maxNOfSols)
            break;
    }
    if(nbSol == 0){
        std::cout << "No solutions" << std::endl;
        write_to_log_file("No solutions found.", LOG_FILE);
    }
    write_to_log_file(statistics_to_string(solver->statistics()).c_str(), LOG_FILE);
    std::cout << statistics_to_string(solver->statistics()) << std::endl;
    return sols;
}