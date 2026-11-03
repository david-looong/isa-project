#ifndef WRITEBACK_HPP
#define WRITEBACK_HPP

#include "hart.hpp"
#include "pipeline_object.hpp"

void writeback_rising_edge(Hart&, HartPipelineObject &hpo);
void writeback_falling_edge(Hart &hart, HartPipelineObject &hpo);

#endif // WRITEBACK_HPP