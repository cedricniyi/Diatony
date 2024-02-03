import matplotlib.pyplot as plt

filename = "out/search-stats-2024-02-02_10-20-31.csv"

data = []
#read file
with open(filename, 'r') as csvfile:
    for line in csvfile.readlines():
        #for each line, separate each element and return a list of lists
        line_data = [elem.strip() for elem in line.split(',')]
        data.append(line_data)

#remove first line with titles
test_cases = data[1:]

#isolate the data we want to plot
chord_progressions = []
tonality = []
times = []
n4_note_dim_chords = []
n_3_note_chords = []
n_fund_chords_no_double_bass = []
n_incomplete_chords = []
n_common_notes_soprano = []
n_common_notes_same_voice = []
melodic_intervals_costs = []

for case in test_cases:
    #initialize lists for each case
    curr_t = []
    curr_dim = []
    curr_3 = []
    curr_fund = []
    curr_inc = []
    curr_soprano = []
    curr_same = []
    curr_mel = []

    # start at 31 to get each successive found solution
    temp = case[31:]

    i = 0
    while i + 8 < len(temp):
        curr_t.append(float(temp[i]))
        curr_dim.append(int(temp[i+1]))
        curr_3.append(int(temp[i+2]))
        curr_fund.append(int(temp[i+3]))
        curr_inc.append(int(temp[i+4]))
        curr_soprano.append(int(temp[i+5]))
        curr_same.append(-int(temp[i+6]))
        curr_mel.append(int(temp[i+7]))
        i+= 9 # move to the next solution
        
    #gather the information for this test case
    chord_progressions.append(case[0])
    tonality.append(case[1])
    times.append(curr_t)
    n4_note_dim_chords.append(curr_dim)
    n_3_note_chords.append(curr_3)
    n_fund_chords_no_double_bass.append(curr_fund)
    n_incomplete_chords.append(curr_inc)
    n_common_notes_soprano.append(curr_soprano)
    n_common_notes_same_voice.append(curr_same)
    melodic_intervals_costs.append(curr_mel)

min_t = min([item for sublist in times for item in sublist])
max_t = max([item for sublist in times for item in sublist])

costs = [n4_note_dim_chords, n_3_note_chords, n_fund_chords_no_double_bass, n_incomplete_chords, n_common_notes_soprano, n_common_notes_same_voice,
        melodic_intervals_costs]
costs_labels = ['Number of 4-note diminished chords', 'Number of 3-note chords', 'Number of fundamental chords without double bass', 'Number of incomplete chords',
          'Number of common notes in the soprano', 'Number of common notes in the same voice','Melodic intervals cost']

# for each cost
for i in range(len(costs)):
    # for each test case
    for j in range(len(chord_progressions)):
        a = 0
        #plot the cost graph for the test case with the appropriate label
        plt.plot(times[j], costs[i][j], label = f'{chord_progressions[j]} - {tonality[j]}')

    plt.xlabel('Time')
    plt.ylabel(costs_labels[i])
    plt.title(f'Evolution of the {costs_labels[i]} over time')

    plt.legend()
    plt.show()
