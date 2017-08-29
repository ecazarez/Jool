#include "translation_state.h"
#include "nat64/bib/entry.h"

void xlation_init(struct xlation *state)
{
	bib_session_init(&state->entries);
	memset(&state->in.debug, 0, sizeof(state->in.debug));
	memset(&state->out.debug, 0, sizeof(state->out.debug));
}

void xlation_put(struct xlation *state)
{
	xlator_put(&state->jool);
}
