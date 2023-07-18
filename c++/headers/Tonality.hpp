#ifndef TONALITY
#define TONALITY

#include <gecode/int.hh>

#include "Utilities.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace Gecode;
using namespace std;

/**
 * This class models a tonality
 * @todo move note values to [0,11] instead of [12,23]
 */
class Tonality { // abstract class
protected:
    int tonic;                              // tonic of the tonality
    int mode;                               // mode of the tonality
    vector<int> scale;                      // scale of the tonality
    map<int, int> degrees_notes;            // notes corresponding to the degrees of the scale
    map<int, vector<int>> chord_qualities;  // map of [degree, chord_quality] for each degree of the scale (0 to 6) (set by child classes)

    map<int, IntSet> scale_degrees;         // map of [degree, all_notes] for each degree of the scale (0 to 6)
    IntSet tonal_notes;                     // notes that don't change in major or minor mode (1,4,5 degrees)
    IntSet modal_notes;                     // notes that change in major or minor mode (3,6,7 degrees)

    map<int, IntSet> scale_degrees_chords;  // map of [degree, chord] for each degree of the scale (0 to 6) (set by child classes)

public:
    /**
     * Constructor
     * @param t the tonic of the tonality
     * @param m the mode of the tonality
     * @param s the scale of the tonality
     */
    Tonality(int t, int m, vector<int> s);

    /**
     * Get the tonic of the tonality
     * @return the tonic of the tonality
     */
    int get_tonic();

    /**
     * Get the mode of the tonality
     * @return the mode of the tonality
     */
    int get_mode();

    /**
     * Get the mode of the tonality
     * @return the mode of the tonality
     */
    vector<int> get_scale();

    /**
     * Get the notes corresponding to the degrees of the scale (first elem = tonic, second_elem = second degree, etc.)
     * @return a map containing the notes for each of the scale degrees
     */
    map<int,int> get_degrees_notes();

    /**
     * Get the note for a given degree
     * @param degree the degree of the scale [0,6]
     * @return the note for the given degree
     * @todo change when the minor thing is done
     */
    int get_degree_note(int degree);

    /**
     * Get all the notes for each scale degree
     * @return a map of [degree, all_notes] for each degree of the scale (0 to 6)
     */
    map<int, IntSet> get_scale_degrees();

    /**
     * Get all the notes for a given scale degree
     * @param degree a degree of the scale [0,6]
     * @return an IntSet containing all the notes for the given scale degree
     */
    IntSet get_scale_degree(int degree);

    /**
     * Get the notes that don't change in major or minor mode (1,4,5 degrees)
     * @return an IntSet containing the tonal notes
     */
    IntSet get_tonal_notes();

    /**
     * Get the notes that change in major or minor mode (3,6,7 degrees)
     * @return an IntSet containing the modal notes
     */
    IntSet get_modal_notes();

    /** ABSTRACT METHODS */

    /**
     * Get the chord quality for each degree
     * @return a map of [degree, chord_quality] for each degree of the scale (0 to 6)
     */
    virtual map<int, vector<int>> get_chord_qualities() = 0;

    /**
     * Get the chord notes for each degree
     * @return a map of [degree, chord] for each degree of the scale (0 to 6)
     */
    virtual map<int, IntSet> get_scale_degrees_chords() = 0;

    /**
     * Get the chord notes for a given degree
     * @param degree a degree of the scale [0,6]
     * @return an IntSet containing the chord notes for the given degree
     */
    virtual IntSet get_scale_degree_chord(int degree) = 0;
};

#endif