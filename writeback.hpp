/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This header files contains the rising edge and falling edge function
 * declarations for the writeback stage of the instruction pipeline.
 * */

#ifndef WRITEBACK_HPP
#define WRITEBACK_HPP

#include "hart.hpp"
#include "pipeline_object.hpp"

void writeback_rising_edge(Hart&, HartPipelineObject &hpo);
void writeback_falling_edge(Hart &hart, HartPipelineObject &hpo);

#endif // WRITEBACK_HPP