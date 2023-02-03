/**
 * @file FourVoiceTexture.cpp
 * @author Sprockeels Damien (damien.sprockeels@uclouvain.be)
 * @brief This class creates a constraint problem to generate a 4 voice texture based on chord names, qualities and bass.
 * The variables are the following:
 * - An array for each voice containing the intervals between them. They are of size n-1 where n is the number of chords.
 * - An array for the chords. It is of size 4*n, and the first 4 variables represent one chord with the voices in ascending order (bass -> tenor -> alto -> soprano).
 *   The arrays are linked together
 * The currently supported constraints are the following :
 *      - The notes are in the right tonality
 *      - The notes are in the given chord
 *      - The seventh degree of a scale can never be doubled
 *      - If two chords in fundamental position are after each other and the interval between their root notes is a second, the other voices need to move in contrary motion to the bass
 * @version 1.1
 * @date 2023-02-01
 *
 */

/**
 *
 * @todo Add the minimisation of the intervals between notes in the same voice for fundamental state chords
 * @todo Update the fundamentalStateThreeNoteChord constraint to include priority
 *
 * @todo Add an IntSet for each note of the scale in the attributes of the class so we don't have to compute it everytime we need it (maybe create a specific object for it?)
 * @todo Keep working on the tritone resolution constraint
 * @todo Write a constraint that the last chord can't contain a tritone (maybe in the specs though)
 * @todo Check with Karim if the constraint for all notes must be present at least once in perfect chords is valid or not
 * @todo Move the tritone resolution constraint to a loop that iterates over intervals since the constraint has to access both the current and future chords
 * @todo Think about the branching strategy once we have enough constraints that it makes sense
 * @todo Ask Karim what limit to put for intervals in a given voice
 */

#include "FourVoiceTexture.h"

/**
 * @brief Construct a new Four Voice Texture object
 *
 * @param size the number of chords
 * @param key the key of the tonality
 * @param mode the mode of the tonality
 * @param chordRoots the roots of the chords
 * @param chordQualities the qualities of the chords
 * @param chordBass the bass of the chords
 */
FourVoiceTexture::FourVoiceTexture(int size, int key, vector<int> mode, vector<int> chordRoots, vector<vector<int>> chordQualities, vector<int> chordBass)
{
    //-------------------------------------------------------------------Initialisation--------------------------------------------------------------------
    n = size;
    key = key;
    mode = mode;
    chordRoots = chordRoots;
    chordQualities = chordQualities;
    chordBass = chordBass;

    fundamentals = IntSet(getAllGivenNote(key));            // Get all the fundamentals
    fourths = IntSet(getAllGivenNote(key + perfectFourth)); // Get all the fourths (a 5 semitone above the key)
    sevenths = IntSet(getAllGivenNote(key + majorSeventh)); // Get all the sevenths (a 11 semitone above the key)

    // The domain of all notes is the set of all the notes from the (key, mode) tonality
    chordsVoicings = IntVarArray(*this, 4 * n, getAllNotesFromTonality(key, mode));

    bassVoiceIntervals = IntVarArray(*this, n - 1, -12, 12);
    tenorVoiceIntervals = IntVarArray(*this, n - 1, -12, 12);
    altoVoiceIntervals = IntVarArray(*this, n - 1, -12, 12);
    sopranoVoiceIntervals = IntVarArray(*this, n - 1, -12, 12);

    // costs
    doublingCosts = IntVarArray(*this, n, NO_COST, FORBIDDEN);
    totalDoublingCost = IntVar(*this, NO_COST, n * FORBIDDEN);

    //---------------------------------------------------------Linking the variables together--------------------------------------------------------------

    // Posts the constraints that the intervals are the difference between 2 consecutive notes for each voice
    for (int i = 0; i < n - 1; ++i)
    {
        rel(*this, bassVoiceIntervals[i] == chordsVoicings[(i + 1) * 4] - chordsVoicings[i * 4]);
        rel(*this, tenorVoiceIntervals[i] == chordsVoicings[((i + 1) * 4) + 1] - chordsVoicings[(i * 4) + 1]);
        rel(*this, altoVoiceIntervals[i] == chordsVoicings[((i + 1) * 4) + 2] - chordsVoicings[(i * 4) + 2]);
        rel(*this, sopranoVoiceIntervals[i] == chordsVoicings[((i + 1) * 4) + 3] - chordsVoicings[(i * 4) + 3]);
    }

    // Posts the constraint that bass[i] <= tenor[i] <= alto[i] <= soprano[i]
    for (int i = 0; i < n; ++i)
        rel(*this, chordsVoicings.slice(4 * i, 1, 4), IRT_LQ);

    //---------------------------------------------------------------------Constraints---------------------------------------------------------------------

    for (int i = 0; i < n; ++i) // For each chord
    {

        IntVarArgs currentChord = chordsVoicings.slice(4 * i, 1, 4); // Current chord

        setToChord(*this, currentChord, chordRoots[i], chordQualities[i], chordBass[i]); // Set the domain of the notes of that chord to possible notes from the chord

        dontDoubleTheSeventh(*this, currentChord, sevenths); // Never double the seventh

        // For perfect chords, each note should be present at least once
        if (chordQualities[i] == MAJOR_CHORD || chordQualities[i] == MINOR_CHORD || chordQualities[i] == AUGMENTED_CHORD || chordQualities[i] == DIMINISHED_CHORD) // If this is a perfect chord
        {
            count(*this, currentChord, getAllGivenNote(chordRoots[i]), IRT_GQ, 1);                                               // The fundamental is present at least once
            count(*this, currentChord, getAllGivenNote(chordRoots[i] + chordQualities[i][0]), IRT_GQ, 1);                        // The third is present at least once
            count(*this, currentChord, getAllGivenNote(chordRoots[i] + chordQualities[i][0] + chordQualities[i][1]), IRT_GQ, 1); // The fifth is present at least once
        }

        // For 3 note chords, double the fundamental in priority
        fundamentalStateThreeNoteChord(*this, currentChord, chordRoots[i], chordQualities[i], chordBass[i], doublingCosts[i]);
    }

    for (int i = 0; i < n - 1; ++i) // For each interval between the chords
    {
        // tritone resolution
        // tritoneResolution(*this, chordsVoicings, key, i, chordQualities[i], fourths, sevenths);

        // Post the rules for moving from a chord in fundamental state to another
        fundamentalStateChordToFundamentalStateChord(*this, i, bassVoiceIntervals, tenorVoiceIntervals, altoVoiceIntervals, sopranoVoiceIntervals, chordBass, chordRoots);
    }

    //----------------------------------------------------------------------Branching----------------------------------------------------------------------

    Rnd r1(0);
    Rnd r2(1);
    branch(*this, chordsVoicings, INT_VAR_RND(r1), INT_VAL_RND(r2));

    /*     branch(*this, sopranoVoiceIntervals, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
        branch(*this, altoVoiceIntervals, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
        branch(*this, tenorVoiceIntervals, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
        branch(*this, bassVoiceIntervals, INT_VAR_SIZE_MIN(), INT_VAL_MIN()); */
}

/**********************************************************************
 *                                                                    *
 *                          Support functions                         *
 *                                                                    *
 **********************************************************************/

/**
 * @brief Print all the variables, used for development
 *
 */
void FourVoiceTexture::printDevelop(void) const
{
    std::cout << "bass : " << bassVoiceIntervals << std::endl
              << "tenor : " << tenorVoiceIntervals << std::endl
              << "alto : " << altoVoiceIntervals << std::endl
              << "soprano : " << sopranoVoiceIntervals << std::endl;

    std::cout << chordsVoicings << std::endl;
}

/**
 * @brief Print the solution
 *
 */
void FourVoiceTexture::print(void) const
{
    // std::for_each(chordsVoicings.begin(), chordsVoicings.end(), printIntVar);
    for (int i = 0; i < chordsVoicings.size(); ++i)
    {
        if (i % 4 == 0 && i != 0)
            std::cout << std::endl;
        printNoteInLetter(chordsVoicings[i]);
    }
}

/**
 * @brief Prints the solution in an OM-ready way (parenthesis with the note values in MIDIcent). Example output : ((6000 6200 6400) (6500 6700 6900))
 *
 */
void FourVoiceTexture::printForOM(void) const
{
    for (int i = 0; i < chordsVoicings.size(); ++i)
    {
        if (i % 4 == 0 && i != 0)
            std::cout << std::endl;
        printNoteForOM(chordsVoicings[i]);
    }
}

/**
 * @brief Search support, updates the variables
 *
 * @param s
 */
FourVoiceTexture::FourVoiceTexture(FourVoiceTexture &s) : Space(s)
{
    bassVoiceIntervals.update(*this, s.bassVoiceIntervals);
    tenorVoiceIntervals.update(*this, s.tenorVoiceIntervals);
    altoVoiceIntervals.update(*this, s.altoVoiceIntervals);
    sopranoVoiceIntervals.update(*this, s.sopranoVoiceIntervals);

    chordsVoicings.update(*this, s.chordsVoicings);
}

/**
 * @brief This method is called when a Branch and Bound solver is used everytime a solution is found by the solver.
 *
 * @param _b The solution found by the solver
 */
void FourVoiceTexture::constrain(const Space &_b)
{
    std::cout << "TODO" << std::endl;
}