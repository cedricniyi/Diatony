#include "headers/FourVoiceTexture.hpp"

/***********************************************************************************************************************
 *                                                                                                                     *
 *                                                                                                                     *
 *                                             FourVoiceTexture class methods                                          *
 *                                                                                                                     *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * Constructor
 * @param s the number of chords in the progression
 * @param *t a pointer to a Tonality object
 * @param chordDegs the degrees of the chord of the chord progression
 * @param chordStas the states of the chord of the chord progression (fundamental, 1st inversion,...)
 * Returns a FourVoiceTexture object
 */
FourVoiceTexture::FourVoiceTexture(int s, Tonality *t, vector<int> chordDegs, vector<int> chordStas){
    /// basic data
    size = s;
    tonality = t;
    chordDegrees = chordDegs;
    chordStates = chordStas;

    /// solution array
    FullChordsVoicing = IntVarArray(*this, nOfVoices * size, 0, 127); // tonality->get_tonality_notes()

    /// variable arrays for melodic intervals for each voice
    bassMelodicIntervals = IntVarArray(*this, size - 1, -PERFECT_OCTAVE, PERFECT_OCTAVE);
    tenorMelodicIntervals = IntVarArray(*this, size - 1, -PERFECT_OCTAVE, PERFECT_OCTAVE);
    altoMelodicIntervals = IntVarArray(*this, size - 1, -PERFECT_OCTAVE, PERFECT_OCTAVE);
    sopranoMelodicIntervals = IntVarArray(*this, size - 1, -PERFECT_OCTAVE, PERFECT_OCTAVE);

    /// variable arrays for absolute melodic intervals for each voice
    absoluteBassMelodicIntervals = IntVarArray(*this, size - 1, 0, PERFECT_OCTAVE);
    absoluteTenorMelodicIntervals = IntVarArray(*this, size - 1, 0, PERFECT_OCTAVE);
    absoluteAltoMelodicIntervals = IntVarArray(*this, size - 1, 0, PERFECT_OCTAVE);
    absoluteSopranoMelodicIntervals = IntVarArray(*this, size - 1, 0, PERFECT_OCTAVE);

    /// variable arrays for harmonic intervals between adjacent voices (only positive because there is no direction)
    bassTenorHarmonicIntervals = IntVarArray(*this, size, 0, PERFECT_OCTAVE + PERFECT_FIFTH);
    bassAltoHarmonicIntervals = IntVarArray(*this, size, 0, 2 * PERFECT_OCTAVE + PERFECT_FIFTH);
    bassSopranoHarmonicIntervals = IntVarArray(*this, size, 0, 3 * PERFECT_OCTAVE + PERFECT_FIFTH);
    tenorAltoHarmonicIntervals = IntVarArray(*this, size, 0, PERFECT_OCTAVE);
    tenorSopranoHarmonicIntervals = IntVarArray(*this, size, 0, 2 * PERFECT_OCTAVE);
    altoSopranoHarmonicIntervals = IntVarArray(*this, size, 0, PERFECT_OCTAVE);

    /// cost variables auxiliary arrays
    nDifferentValuesInDiminishedChord = IntVarArray(*this, size, 0, nOfVoices);
    nDifferentValuesAllChords = IntVarArray(*this, size, 0, nOfVoices);
    nOccurrencesBassInFundamentalState = IntVarArray(*this, size, 0, nOfVoices);
    commonNotesInSoprano = IntVarArray(*this, size-1, 0, 1); // 1 because if it is at the bass we can't prevent it, so it only counts for the soprano

    /// cost variables
    sumOfMelodicIntervals = IntVar(*this, 0, PERFECT_OCTAVE * nOfVoices * (size - 1));
    nOfDiminishedChordsWith4notes = IntVar(*this, 0, size);
    nOfChordsWithLessThan4notes = IntVar(*this, 0, size);
    nOfFundamentalStateChordsWithoutDoubledBass = IntVar(*this, 0, size);
    nOfCommonNotesInSoprano = IntVar(*this, 0, size);


    /// print parameters to log file
    write_to_log_file(parameters().c_str());

    /// Test constraints

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    |                                              link the arrays together                                            |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/

    link_melodic_arrays(*this, size, nOfVoices, FullChordsVoicing, bassMelodicIntervals,
                        altoMelodicIntervals, tenorMelodicIntervals, sopranoMelodicIntervals);

    link_absolute_melodic_arrays(*this, bassMelodicIntervals, tenorMelodicIntervals, altoMelodicIntervals,
                                 sopranoMelodicIntervals, absoluteBassMelodicIntervals, absoluteTenorMelodicIntervals,
                                 absoluteAltoMelodicIntervals, absoluteSopranoMelodicIntervals);

    link_harmonic_arrays(*this, size, nOfVoices, FullChordsVoicing, bassTenorHarmonicIntervals,
                         bassAltoHarmonicIntervals, bassSopranoHarmonicIntervals, tenorAltoHarmonicIntervals,
                         tenorSopranoHarmonicIntervals, altoSopranoHarmonicIntervals);

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    |                                              generic constraints                                                 |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/

    /// restrain the domain of the voices to their range + state that bass <= tenor <= alto <= soprano
    restrain_voices_domains(*this, size, nOfVoices, FullChordsVoicing);

    for(int i = 0; i < size; i++) {
        IntVarArgs currentChord(FullChordsVoicing.slice(nOfVoices * i, 1, nOfVoices));

        /// set the chord's domain to the notes of the degree chordDegrees[i]'s chord
        set_to_chord(*this, tonality, chordDegrees[i], currentChord);

        /// set the bass based on the chord's state
        set_bass(*this, tonality, chordDegrees[i], chordStates[i], currentChord);
    }

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    |                                             set up costs computation                                             |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/

    // @todo add a cost for doubled notes that are not tonal notes

    /// number of diminished chords in fundamental state with more than 3 notes (cost to minimize)
    compute_diminished_chords_cost(*this, size, nOfVoices, tonality, chordDegrees,
                                   chordStates, FullChordsVoicing,
                                   nDifferentValuesInDiminishedChord,
                                   nOfDiminishedChordsWith4notes);

    /// number of chords with less than 4 note values (cost to minimize)
    compute_n_of_notes_in_chord_cost(*this, size, nOfVoices, FullChordsVoicing,
                                     nDifferentValuesAllChords,nOfChordsWithLessThan4notes);

    /// number of fundamental state chords without doubled bass (cost to minimize)
    compute_fundamental_state_doubling_cost(*this, size, nOfVoices, tonality,
                                            chordDegrees, chordStates, FullChordsVoicing,
                                            nOccurrencesBassInFundamentalState,
                                            nOfFundamentalStateChordsWithoutDoubledBass);

    /// number of common notes in soprano with first inversion chord going to another chord (cost to minimize)
    compute_cost_for_common_note_in_soprano(*this, size, nOfVoices, chordStates, FullChordsVoicing,
                                            commonNotesInSoprano,
                                            nOfCommonNotesInSoprano);

    /// sum of melodic intervals (cost to minimize)
    linear(*this, IntVarArgs() << absoluteTenorMelodicIntervals << absoluteAltoMelodicIntervals <<
                               absoluteSopranoMelodicIntervals << absoluteBassMelodicIntervals,
                               IRT_EQ, sumOfMelodicIntervals);

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    |     Harmonic constraints: loop over each chord and post the constraints depending on the degree and state        |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/

    for(int i = 0; i < size; i++){
        IntVarArgs currentChord(FullChordsVoicing.slice(nOfVoices * i, 1, nOfVoices));

        /// post the constraints depending on the chord's state
        if(chordStas[i] == FUNDAMENTAL_STATE){
            /// each note should be present at least once, doubling is determined with costs
            chord_note_occurrence_fundamental_state(*this, nOfVoices, chordDegrees[i], tonality,
                                                    currentChord,
                                                    nDifferentValuesInDiminishedChord[i]);
        }
        /// post the constraints specific to first inversion chords
        else if(chordStas[i] == FIRST_INVERSION){
            chord_note_occurrence_first_inversion(*this, size, nOfVoices, i, tonality,
                                                  chordDegrees, currentChord, bassMelodicIntervals,
                                                  sopranoMelodicIntervals);
        }
        /// post the constraints specific to second inversion chords
        else if(chordStas[i] == SECOND_INVERSION){

        }
        else{
            //@todo add the other cases here (4 note chords, etc)
        }
    }

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    | Melodic constraints: loop over each space between chords and post the constraints depending on the state and     |`
    | quality of the chords                                                                                            |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/

    /// between each chord
    for(int i = 0; i < size-1; i++) {
        /// parallel unissons, fifths and octaves are forbidden unless we have the same chord twice in a row
        if(chordDegrees[i] != chordDegrees[i + 1]){
            forbid_parallel_intervals(*this, size, nOfVoices, {PERFECT_FIFTH, PERFECT_OCTAVE, UNISSON},
                                      FullChordsVoicing,
                                      bassTenorHarmonicIntervals, bassAltoHarmonicIntervals,
                                      bassSopranoHarmonicIntervals,
                                      tenorAltoHarmonicIntervals, tenorSopranoHarmonicIntervals,
                                      altoSopranoHarmonicIntervals);
        }

        /// resolve the tritone if there is one and it needs to be resolved
        if (chordDegs[i] == SEVENTH_DEGREE && chordDegs[i + 1] == FIRST_DEGREE) {
            //@todo add other chords that have the tritone
            tritone_resolution(*this, nOfVoices, i, tonality,
                               bassMelodicIntervals, tenorMelodicIntervals,
                               altoMelodicIntervals, sopranoMelodicIntervals,
                               FullChordsVoicing);
        }

        /// Exceptions to the general voice leading rules

        /// special rule for interrupted cadence
        if (chordDegs[i] == FIFTH_DEGREE && chordStas[i] == FUNDAMENTAL_STATE &&
        chordDegs[i + 1] == SIXTH_DEGREE && chordStas[i + 1] == FUNDAMENTAL_STATE) {
            interrupted_cadence(*this, i, tonality,
                                FullChordsVoicing, tenorMelodicIntervals,
                                altoMelodicIntervals, sopranoMelodicIntervals);
        }
        /// general voice leading rules
        else {
            /// If the bass moves by a step, other voices should move in contrary motion
            int bassFirstChord = (tonality->get_degree_note(chordDegrees[i] + 2 * chordStates[i])
                    % PERFECT_OCTAVE);
            int bassSecondChord = (tonality->get_degree_note(chordDegrees[i + 1] + 2 * chordStates[i + 1])
                    % PERFECT_OCTAVE);
            int bassMelodicMotion = abs(bassSecondChord - bassFirstChord);
            /// if the bass moves by a step between fund. state chords @todo check if this needs to apply in other cases
            if ((bassMelodicMotion == MINOR_SECOND || bassMelodicMotion == MAJOR_SECOND ||
                bassMelodicMotion == MINOR_SEVENTH || bassMelodicMotion == MAJOR_SEVENTH) &&
                (chordStas[i] == FUNDAMENTAL_STATE && chordStas[i+1] == FUNDAMENTAL_STATE)){
                /// move other voices in contrary motion
                contrary_motion_to_bass(*this, i,bassMelodicIntervals,
                                        tenorMelodicIntervals,altoMelodicIntervals,
                                        sopranoMelodicIntervals);
            }
            /// if II -> V, move voices in contrary motion to bass
            else if(chordDegs[i] == SECOND_DEGREE && chordDegs[i+1] == FIFTH_DEGREE){
                contrary_motion_to_bass(*this, i, bassMelodicIntervals,
                                        tenorMelodicIntervals, altoMelodicIntervals,
                                        sopranoMelodicIntervals);
            }
            else if(chordDegrees[i] != chordDegrees[i + 1]){
                /// Otherwise, keep common notes in the same voice
                keep_common_notes_in_same_voice(*this, nOfVoices, i, chordDegrees, tonality,
                                                FullChordsVoicing);
            }
            /// move voices as closely as possible (implemented with costs)
        }
    }

    /**-----------------------------------------------------------------------------------------------------------------
    |                                                                                                                  |
    |                                                       Branching                                                  |
    |                                                                                                                  |
    -------------------------------------------------------------------------------------------------------------------*/
    // @todo make it smarter when it becomes necessary
    branch(*this, FullChordsVoicing, INT_VAR_DEGREE_MAX(), INT_VAL_MIN());
}

/**
 * Cost function for lexicographical minimization. The order is as follows:
 * 1. number of diminished chords with more than 3 notes
 * 2. number of chords with less than 4 note values
 * 3. number of fundamental state chords without doubled bass
 * 4. sum of melodic intervals minimizes the melodic movement between chords
 * @return the cost variables in order of importance
 */
IntVarArgs FourVoiceTexture::cost() const {
    // @todo maybe give the voices a priority + check the order depending on what is more important
    return {nOfDiminishedChordsWith4notes, nOfChordsWithLessThan4notes, nOfFundamentalStateChordsWithoutDoubledBass,
            sumOfMelodicIntervals, nOfCommonNotesInSoprano};
}

/**
 * Copy constructor
 * @param s an instance of the FourVoiceTexture class
 * @return a copy of the given instance of the FourVoiceTexture class
 */
FourVoiceTexture::FourVoiceTexture(FourVoiceTexture& s): IntLexMinimizeSpace(s){
    size = s.size;
    tonality = s.tonality;
    chordDegrees = s.chordDegrees;
    chordStates = s.chordStates;

    bassMelodicIntervals.update(*this, s.bassMelodicIntervals);
    tenorMelodicIntervals.update(*this, s.tenorMelodicIntervals);
    altoMelodicIntervals.update(*this, s.altoMelodicIntervals);
    sopranoMelodicIntervals.update(*this, s.sopranoMelodicIntervals);

    absoluteBassMelodicIntervals.update(*this, s.absoluteBassMelodicIntervals);
    absoluteTenorMelodicIntervals.update(*this, s.absoluteTenorMelodicIntervals);
    absoluteAltoMelodicIntervals.update(*this, s.absoluteAltoMelodicIntervals);
    absoluteSopranoMelodicIntervals.update(*this, s.absoluteSopranoMelodicIntervals);

    bassTenorHarmonicIntervals.update(*this, s.bassTenorHarmonicIntervals);
    tenorAltoHarmonicIntervals.update(*this, s.tenorAltoHarmonicIntervals);
    altoSopranoHarmonicIntervals.update(*this, s.altoSopranoHarmonicIntervals);

    FullChordsVoicing.update(*this, s.FullChordsVoicing);

    nDifferentValuesInDiminishedChord.update(*this, s.nDifferentValuesInDiminishedChord);
    nDifferentValuesAllChords.update(*this, s.nDifferentValuesAllChords);
    nOccurrencesBassInFundamentalState.update(*this, s.nOccurrencesBassInFundamentalState);
    commonNotesInSoprano.update(*this, s.commonNotesInSoprano);

    sumOfMelodicIntervals.update(*this, s.sumOfMelodicIntervals);
    nOfDiminishedChordsWith4notes.update(*this, s.nOfDiminishedChordsWith4notes);
    nOfChordsWithLessThan4notes.update(*this, s.nOfChordsWithLessThan4notes);
    nOfFundamentalStateChordsWithoutDoubledBass.update(*this, s.nOfFundamentalStateChordsWithoutDoubledBass);
    nOfCommonNotesInSoprano.update(*this, s.nOfCommonNotesInSoprano);
}

/**
 * Returns the size of the problem
 * @return an integer representing the size of the vars array
 */
int FourVoiceTexture::get_size() const{
    return size;
}

/**
 * Copy method
 * @return a copy of the current instance of the FourVoiceTexture class. Calls the copy constructor
 */
Space* FourVoiceTexture::copy() {
    return new FourVoiceTexture(*this);
}

/**
 * Returns the values taken by the variables vars in a solution
 * @return an array of integers representing the values of the variables in a solution
 */
int* FourVoiceTexture::return_solution(){
    int* solution = new int[size*4];
    for(int i = 0; i < 4*size; i++){
        solution[i] = FullChordsVoicing[i].val();
    }
    return solution;
}

/**
 * Constrain method for bab search
 * not needed for optimization problems as it is already implemented
 * @param _b a solution to the problem from which we wish to add a constraint for the next solutions
 */
//void FourVoiceTexture::constrain(const Space& _b) {
//    const auto &b = dynamic_cast<const FourVoiceTexture &>(_b);
//}

/**
 * Prints the solution in the console
 */
void FourVoiceTexture::print_solution(){
    for(int i = 0; i < 4*size; i++){
        cout << FullChordsVoicing[i].val() << " ";
    }
    cout << endl;
}

/**
     * returns the parameters in a string
     * @return a string containing the parameters of the problem
     */
string FourVoiceTexture::parameters(){
    string message = "-----------------------------------------parameters-----------------------------------------\n";
    message += "Number of chords: " + std::to_string(size) + "\n";
    message += "Tonality: " + midi_to_letter(tonality->get_tonic()) + " " + mode_int_to_name(tonality->get_mode()) + "\n";
    message += "Chords: \n";
    for(int i = 0; i < size; i++){
        message += degreeNames[chordDegrees[i]] + " in " +
                   stateNames[chordStates[i]];
        if(i != size - 1)
            message += ",\n";
    }
    return message + ".\n";
}

/**
 * toString method
 * @todo make a toString method for IntVarArrays so the code here is cleaner
 * @return a string representation of the current instance of the FourVoiceTexture class.
 * Right now, it returns a string "FourVoiceTexture object. size = <size>"
 * If a variable is not assigned when this function is called, it writes <not assigned> instead of the value
 */
string FourVoiceTexture::to_string(){
    string message;
    message += "********************************************************************************************\n";
    message += "*                                                                                          *\n";
    message += "*                                          Solution                                        *\n";
    message += "*                                                                                          *\n";
    message += "********************************************************************************************\n\n";
    message += parameters();

    message += "\n-----------------------------------------variables------------------------------------------\n";

    message += "BassTenorHarmonicIntervals = " + intVarArray_to_string(bassTenorHarmonicIntervals) + "\n";
    message += "TenorAltoHarmonicIntervals = " + intVarArray_to_string(tenorAltoHarmonicIntervals) + "\n";
    message += "AltoSopranoHarmonicIntervals = " + intVarArray_to_string(altoSopranoHarmonicIntervals) + "\n\n";

    message += "BassMelodicIntervals = " + intVarArray_to_string(bassMelodicIntervals) + "\n";
    message += "TenorMelodicIntervals = " + intVarArray_to_string(tenorMelodicIntervals) + "\n";
    message += "AltoMelodicIntervals = " + intVarArray_to_string(altoMelodicIntervals) + "\n";
    message += "SopranoMelodicIntervals = " + intVarArray_to_string(sopranoMelodicIntervals) + "\n\n";

    message += "absoluteBassMelodicIntervals = " + intVarArray_to_string(absoluteBassMelodicIntervals) + "\n";
    message += "absoluteTenorMelodicIntervals = " + intVarArray_to_string(absoluteTenorMelodicIntervals) + "\n";
    message += "absoluteAltoMelodicIntervals = " + intVarArray_to_string(absoluteAltoMelodicIntervals) + "\n";
    message += "absoluteSopranoMelodicIntervals = " + intVarArray_to_string(absoluteSopranoMelodicIntervals) + "\n\n";

    message += "🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵\n\n";
    message += "FullChordsVoicing = " + intVarArray_to_string(FullChordsVoicing) + "\n\n";
    message += "🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵🎵\n\n";

    message += "-------------------------------cost-related auxiliary arrays------------------------------\n";

    message += "nDifferentValuesInDiminishedChord = " + intVarArray_to_string(nDifferentValuesInDiminishedChord) + "\n";
    message += "nDifferentValuesInAllChords = " + intVarArray_to_string(nDifferentValuesAllChords) + "\n";
    message += "nOccurrencesBassInFundamentalState = " + intVarArray_to_string(nOccurrencesBassInFundamentalState) + "\n\n";
    message += "nCommonNotesInSoprano = " + intVarArray_to_string(commonNotesInSoprano) + "\n\n";

    message += "------------------------------------cost variables----------------------------------------\n";

    message += "nOfDiminishedChordsWith4notes = " + intVar_to_string(nOfDiminishedChordsWith4notes) + "\n";
    message += "nOfChordsWithLessThan4notes = " + intVar_to_string(nOfChordsWithLessThan4notes) + "\n";
    message += "nOfFundamentalStateChordsWithoutDoubledBass = " +
            intVar_to_string(nOfFundamentalStateChordsWithoutDoubledBass) + "\n";
    message += "nOfCommonNotesInSoprano = " + intVar_to_string(nOfCommonNotesInSoprano) + "\n";
    message += "sumOfMelodicIntervals = " + intVar_to_string(sumOfMelodicIntervals) + "\n\n";

    return message;
}