#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "hart.hpp"
#include "pipeline_object.hpp"
#include "sign_extend.hpp"

void memory_rising_edge(Hart &hart, HartPipelineObject &hpo);
void memory_falling_edge(Hart &hart, HartPipelineObject &hpo);

#endif // MEMORY_HPP