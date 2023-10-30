#include "headers/HarmonicConstraints.hpp"

/***********************************************************************************************************************
 *                                                                                                                     *
 * This file contains all the harmonic constraints, that is constraints within a given chord.                          *
 * It currently contains the following constraints:                                                                    *
 *      - set_to_chord: sets the domain of the current chord based on the tonality and the scale degree                *
 *      - set_bass: sets the bass of the chord according to the state of the chord                                     *
 *      - chord_note_occurrence_fundamental_state: sets the number of times each note in the chord are present for     *
 *          fundamental state chords                                                                                   *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * Set the notes of the currentChord array to the notes of the given chord
 * @param home the instance of the problem
 * @param tonality the tonality of the piece
 * @param degree the degree of the chord
 * @param currentChord the array containing a chord in the form [bass, alto, tenor, soprano]
 */
void set_to_chord(const Home& home, Tonality *tonality, int degree, const IntVarArgs& currentChord){
    dom(home, currentChord, tonality->get_scale_degree_chord(degree));
}

/**
 * Set the bass of the chord to be the given note
 * @param home the instance of the problem
 * @param tonality the tonality of the piece
 * @param degree the degree of the chord
 * @param state the state of the chord
 * @param currentChord the array containing a chord in the form [bass, alto, tenor, soprano]
 */
void set_bass(const Home& home, Tonality *tonality, int degree, int state, IntVarArgs currentChord){
    /// (degree + 2) * state gives the bas note of the chord since the state is 0 for fundamental, 1 for first inversion
    /// and 2 for second inversion
    dom(home, currentChord[0], tonality->get_scale_degree((degree + 2 * state) % 7));
}

/***********************************************************************************************************************
 *                                                                                                                     *
 *                                          Fundamental state chord constraints                                        *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * @todo change this for complete and incomplete chords later (third must be <=1 depending on the chord before and
 * @todo after if they are 5->1 and complete/incomplete)
 * @todo maybe make it a preference later
 * Sets the number of times each note of the notes of the chord are present in the chord
 * @param home the instance of the problem
 * @param nVoices the number of voices
 * @param degree the degree of the chord
 * @param tonality the tonality of the piece
 * @param currentChord the array containing a chord in the form [bass, alto, tenor, soprano]
 * @param nDifferentValuesInDiminishedChord the number of different values in the diminished chord
 */
void chord_note_occurrence_fundamental_state(Home home, int nVoices, int degree, Tonality *tonality,
                                             const IntVarArgs &currentChord,
                                             const IntVar &nDifferentValuesInDiminishedChord) {
    /// if the chord is a diminished seventh degree, the third must be doubled
    if(degree == SEVENTH_DEGREE && tonality->get_chord_qualities()[degree] == DIMINISHED_CHORD){
        IntVar nOfThirds(home,0,nVoices);
        count(home, currentChord, tonality->get_scale_degree(degree), IRT_EQ,nOfThirds);
        rel(home, expr(home, nDifferentValuesInDiminishedChord == nVoices), BOT_EQV,
            expr(home, nOfThirds == 2), true);
    }
    /// each note is present at least once, doubling is determined by the costs
    count(home, currentChord, tonality->get_scale_degree(degree), IRT_GQ,1);
    count(home, currentChord, tonality->get_scale_degree((degree + 2) % 7), IRT_GQ,1);
    count(home, currentChord, tonality->get_scale_degree((degree + 4) % 7), IRT_GQ, 1);
}

/***********************************************************************************************************************
 *                                                                                                                     *
 *                                            First inversion chord constraints                                        *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/**
 * Sets the number of time each note of the chord are present in the chord
 * @param home the instance of the problem
 * @param tonality the tonality of the piece
 * @param degrees the degree of the chord
 * @param currentChord the array containing a chord in the form [bass, alto, tenor, soprano]
 */
void chord_note_occurrence_first_inversion(Home home, int size, int nVoices, int currentPos, Tonality *tonality,
                                           vector<int> degrees, const IntVarArgs &currentChord,
                                           IntVarArray bassMelodicIntervals, IntVarArray sopranoMelodicIntervals) {
    /// if the third is a tonal note, then double it
    set<int> tonalNotes = tonality->get_tonal_notes();
    if(tonalNotes.find(tonality->get_degree_note((degrees[currentPos] + THIRD_DEGREE) % 7)) !=
       tonalNotes.end()) { /// double the third and other notes should be present at least once
        count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + THIRD_DEGREE) % 7), IRT_EQ, 2);
    }
    /// if the chord is the seventh degree diminished chord @todo make the difference between b7 and 7° chords
    else if(degrees[currentPos] == SEVENTH_DEGREE) { /// double the third and other notes should be present at least once
        count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + THIRD_DEGREE) % 7), IRT_EQ, 2);
    }
    else{ /// default case: double the fundamental or the fifth of the chord unless the top and bottom voices move down and up respectively
        if(currentPos < size-1 && currentPos > 0){ /// this special case cannot happen on the first and last chord
            ///BoolVar to see if the bass rises for the first motion
            BoolVar bassRises1 = expr(home, expr(home,bassMelodicIntervals[currentPos-1] > 0) &&
                expr(home, bassMelodicIntervals[currentPos-1] <= 2));
            /// BoolVar to see if the bass rises for the second motion
            BoolVar bassRises2 = expr(home, expr(home, bassMelodicIntervals[currentPos] > 0) &&
                expr(home, bassMelodicIntervals[currentPos] <= 2));

            /// BoolVar to see if the bass rises overall
            BoolVar bassRisesOverall = expr(home, bassRises1 && bassRises2);

            /// BoolVar to see if the soprano rises for the first motion
            BoolVar sopranoFalls1 = expr(home, expr(home, sopranoMelodicIntervals[currentPos-1] < 0) &&
                expr(home, sopranoMelodicIntervals[currentPos-1] >= -2));
            /// BoolVar to see if the soprano rises for the second motion
            BoolVar sopranoFalls2 = expr(home, expr(home, sopranoMelodicIntervals[currentPos] < 0) &&
                expr(home, sopranoMelodicIntervals[currentPos] >= -2));

            /// BoolVar to see if the soprano rises overall
            BoolVar sopranoFallsOverall = expr(home, sopranoFalls1 && sopranoFalls2);

            /// BoolVar to see if the voices move step wise by contrary motion over the three chords
            BoolVar contraryMotion = expr(home, bassRisesOverall && sopranoFallsOverall);

            IntVar nOfBassNotes(home,0,nVoices);
            count(home, currentChord, tonality->get_scale_degree(degrees[currentPos] + THIRD_DEGREE), IRT_EQ,nOfBassNotes);
            rel(home, contraryMotion, BOT_EQV, expr(home, nOfBassNotes == 2), true);
            rel(home, expr(home, !contraryMotion), BOT_EQV, expr(home, nOfBassNotes == 1), true);
        }
        else{ /// the bass can't be doubled
            count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + THIRD_DEGREE) % 7), IRT_EQ, 1);
        }
    }
    /// each note always has to be present at least once
    count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + FIRST_DEGREE) % 7), IRT_GQ, 1);
    count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + THIRD_DEGREE) % 7), IRT_GQ, 1);
    count(home, currentChord, tonality->get_scale_degree((degrees[currentPos] + FIFTH_DEGREE) % 7), IRT_GQ, 1);
}