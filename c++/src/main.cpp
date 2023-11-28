#include "../headers/FourVoiceTexture.hpp"
#include "../headers/Utilities.hpp"
#include "../headers/Tonality.hpp"
#include "../headers/MajorTonality.hpp"
#include "../headers/MinorTonality.hpp"
#include "../headers/Solver.hpp"
#include "../headers/MidiFile.h"

using namespace Gecode;
using namespace std;
using namespace smf;

int main(int argc, char* argv[]) {
    Tonality* tonality = new MinorTonality(C);

    write_to_log_file(time().c_str());
//    vector<int> chords = {FIRST_DEGREE, FOURTH_DEGREE, SEVENTH_DEGREE,
//                          THIRD_DEGREE, SIXTH_DEGREE, SECOND_DEGREE, FIRST_DEGREE,
//                          FIFTH_DEGREE, FIRST_DEGREE};
//
//    vector<int> states = {FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FIRST_INVERSION, FUNDAMENTAL_STATE,
//                          FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, SECOND_INVERSION, FUNDAMENTAL_STATE,
//                          FUNDAMENTAL_STATE};
//    vector<int> chords = {FIRST_DEGREE, FIFTH_DEGREE, SIXTH_DEGREE, FIRST_DEGREE, FIFTH_DEGREE, FIRST_DEGREE, THIRD_DEGREE,
//                          SIXTH_DEGREE, SECOND_DEGREE, FIFTH_DEGREE, FIRST_DEGREE, FOURTH_DEGREE, FIFTH_DEGREE, FIFTH_DEGREE,
//                          FIRST_DEGREE};
//    vector<int> states = {FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, SECOND_INVERSION, FUNDAMENTAL_STATE,
//                          FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FIRST_INVERSION, FUNDAMENTAL_STATE,
//                          FIRST_INVERSION, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE};
    vector<int> chords = {FIRST_DEGREE, SIXTH_DEGREE, FOURTH_DEGREE, FIRST_DEGREE, FIFTH_DEGREE, FIRST_DEGREE,
                          SECOND_INVERSION, FIRST_DEGREE, FIFTH_DEGREE, FIRST_DEGREE};
    vector<int> states = {FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE, SECOND_INVERSION, FUNDAMENTAL_STATE,
                          FIRST_INVERSION, FIRST_INVERSION, SECOND_INVERSION, FUNDAMENTAL_STATE, FUNDAMENTAL_STATE};
    int size = chords.size();

    /// create a new problem
    auto *pb = new FourVoiceTexture(size, tonality, chords, states);

    /// find the solution that minimizes the costs (maximize the preference satisfaction)
    // const FourVoiceTexture *bestSol = find_best_solution(pb);
    /// find all solutions to the problem
    auto start = std::chrono::high_resolution_clock::now();/// start time
    auto sols = find_all_solutions(pb, BAB_SOLVER);
    auto end = std::chrono::high_resolution_clock::now();/// end time

    delete pb;

    /// total time spent searching
    std::chrono::duration<double> duration = end - start;
    std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;

    ///-------------------------------------------create the MIDI file-----------------------------------------------///
    MidiFile outputFile; // create an empty MIDI file with one track
    outputFile.absoluteTicks(); // time information stored as absolute time, will be converted to delta time when written

    outputFile.addTrack(1);   // Add a track to the file (track 0 must be left empty, so add as many as we use
    vector<uchar> midiEvent;   // temporary storage of MIDI events
    midiEvent.resize(3);   //set the size of the array to 3 bites
    int tpq = 120;            // default value in MIDI file is 48
    outputFile.setTicksPerQuarterNote(tpq);

    /// get the best solution
    //@todo fix this to get the notes from the solution
    auto bestSolution = sols[sols.size() - 1]; // the last one is the best
    int* notes = bestSolution->return_solution();
    int rhythm[size];

    for(int i = 0; i < 4*size; i++)
        rhythm[i] = 1;

    /// Fill the MidiFile object
    int i = 0;
    int actionTime = 0; // temporary storage for MIDI event time
    midiEvent[2] = 64; // store attack/release velocity for note command
    while (notes[i] >= 0) {
        midiEvent[0] = 0x90;     // store a note on command (MIDI channel 1)
        midiEvent[1] = notes[i];
        outputFile.addEvent(1, actionTime, midiEvent);
        actionTime += tpq * rhythm[i];
        midiEvent[0] = 0x80;     // store a note on command (MIDI channel 1)
        outputFile.addEvent(1, actionTime, midiEvent);
        i++;
    }

    outputFile.sortTracks(); // make sure data is in correct order
    outputFile.write("../out/output.mid");


    return 0;
}