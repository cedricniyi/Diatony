#ifndef MYPROJECT_SOLVER_HPP
#define MYPROJECT_SOLVER_HPP

#include "FourVoiceTexture.hpp"
#include "Utilities.hpp"
#include "Tonality.hpp"
#include "MajorTonality.hpp"
#include "MinorTonality.hpp"

using namespace Gecode;
using namespace std;

/***********************************************************************************************************************
 *                                                                                                                     *
 *                                                Search engine methods                                                *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * This file contains all the functions responsible for creating and using search engines for the FourVoiceTexture class.
 */

/**
 * Creates a search engine for the given problem
 * @param pb an instance of the FourVoiceTexture class representing a given problem
 * @param type the type of search engine to create (see enumeration in headers/gecode_problem.hpp)
 * @param timeout the maximum time in milliseconds the solver can run for (default value is 60000)
 * @return a search engine for the given problem
 */
Base<FourVoiceTexture> *make_solver(FourVoiceTexture *pb, int type, int timeout);

/**
 * Returns the next solution space for the problem
 * @param solver a solver for the problem
 * @return an instance of the FourVoiceTexture class representing the next solution to the problem
 */
FourVoiceTexture* get_next_solution_space(Search::Base<FourVoiceTexture>* solver);

/**
 * Returns the best solution for the problem pb. It uses a branch and bound solver with lexico-minimization of the costs
 * @param pb an instance of a FourVoiceTexture problem
 * @param timeout the maximum time in milliseconds the solver can run for (default value is 60000)
 * @return the best solution to the problem
 */
const FourVoiceTexture * find_best_solution(FourVoiceTexture *pb, int timeout = 60000,
                                            string csvFileName = STATISTICS_CSV + ".csv", string preMessage = "");

string best_sol_stats(FourVoiceTexture *pb, int timeout = 60000, string csvFileName = STATISTICS_CSV + ".csv",
                      string preMessage = "");

/**
 * Returns the first maxNOfSols solutions for the problem pb using the solver solverType.
 * @param pb an instance of a FourVoiceTexture problem
 * @param solverType the type of the solver to use from solver_types
 * @param maxNOfSols the maximum number of solutions we want to find (the default value is MAX_INT)
 * @return the first maxNOfSols solutions to the problem
 */
vector<const FourVoiceTexture *> find_all_solutions(FourVoiceTexture *pb, int solverType,
                                                    int maxNOfSols = std::numeric_limits<int>::max(),
                                                    int timeout = 60000);

#endif
