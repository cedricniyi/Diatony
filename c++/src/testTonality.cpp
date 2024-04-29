#include "diatony/FourVoiceTexture.hpp"
#include "aux/Utilities.hpp"
#include "aux/Tonality.hpp"
#include "aux/MajorTonality.hpp"
#include "aux/MinorTonality.hpp"

using namespace Gecode;
using namespace std;

int main(int argc, char* argv[]) {
    std::cout << "Tests of Tonality and MajorTonality classes.\n\n";
    Tonality* tonality = new MajorTonality(C);

    std::cout << "\n\nTonic : " << tonality->get_degree_note(FIRST_DEGREE) << ", leading tone : " << tonality->get_degree_note(SEVENTH_DEGREE) << std::endl;
    std::cout << "Expected : " << to_string(C) << ", " << to_string(B) << std::endl;

    std::cout << "\n Tonic values : " << tonality->get_degree_note(FIRST_DEGREE) << ", degrees_notes[" << FIFTH_DEGREE + 1 << "] : "
    << tonality->get_degree_note(FIFTH_DEGREE) << "]" << std::endl;

    std::cout << "\n Tonic values : " << tonality->get_scale_degree(FIRST_DEGREE) << ", degrees_notes[" << FIFTH_DEGREE + 1 << " : "
    << tonality->get_scale_degree(FIFTH_DEGREE) << "]" << std::endl;

    std::cout << "\n Tonal notes : ";
    for(int elem : tonality->get_tonal_notes())
        std::cout << elem << " ";
    std::cout << std::endl;

    std::cout << "\n Modal notes : ";
    for(int elem : tonality->get_modal_notes())
        std::cout << elem << " ";
    std::cout << std::endl;

    std::cout << "\n Dominant chord : " << tonality->get_scale_degree_chord(FIFTH_DEGREE) << ", leading tone chord : "
    << tonality->get_scale_degree_chord(SEVENTH_DEGREE) << std::endl;

    return 0;
}
