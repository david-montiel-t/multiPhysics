#include "../../include/multiPhysicsBVP.h"

#include "../../include/FloodFiller.h"
#include "../../include/SimplifiedGrainRepresentation.h"
#include "../../include/OrderParameterRemapper.h"


//vmult operation for LHS
template <int dim, int degree>
void MultiPhysicsBVP<dim,degree>::reassignGrains () {

    //log time
    computing_timer_pf.enter_subsection("multiPhysicsBVP: reassignGrains");

    pcout << "Reassigning grains..." << std::endl;

    // Get the index of the first scalar field (used to get the FE object and DOFHandler)
    unsigned int scalar_field_index = 0;
    for (unsigned int var=0; var<userInputs_pf.number_of_variables; var++){
        if (userInputs_pf.var_type.at(var) == SCALAR){
            scalar_field_index = var;
            break;
        }
    }

    // Create the simplified grain representations
    QGaussLobatto<dim> quadrature2 (degree+1);
    FloodFiller<dim, degree> flood_filler(*FESet.at(scalar_field_index), quadrature2);

    std::vector<GrainSet<dim>> grain_sets;

    unsigned int op_list_index = 0;
    for(unsigned int fieldIndex=0; fieldIndex<fields.size(); fieldIndex++){
        if (op_list_index < userInputs_pf.variables_for_remapping.size()){
            if (fieldIndex == userInputs_pf.variables_for_remapping.at(op_list_index)){
                op_list_index++;

                std::vector<GrainSet<dim>> single_OP_grain_sets;
                flood_filler.calcGrainSets(*FESet.at(scalar_field_index), *dofHandlersSet_nonconst.at(scalar_field_index), solutionSet.at(fieldIndex), userInputs_pf.order_parameter_threshold, 1.0+userInputs_pf.order_parameter_threshold, fieldIndex, single_OP_grain_sets);

                grain_sets.insert(grain_sets.end(), single_OP_grain_sets.begin(), single_OP_grain_sets.end());
            }
        }
    }

    // Set the grain indices to unique values
    for (unsigned int g=0; g<grain_sets.size(); g++){
        grain_sets.at(g).setGrainIndex(g);
    }

    std::vector<SimplifiedGrainRepresentation<dim>> old_grain_representations = simplified_grain_representations;
    simplified_grain_representations.clear();
    for (unsigned int g=0; g<grain_sets.size(); g++){
        SimplifiedGrainRepresentation<dim> simplified_grain_representation(grain_sets.at(g));

        pcout << "Grain: " << simplified_grain_representation.getGrainId() << " " << simplified_grain_representation.getOrderParameterId() << " Center: " << simplified_grain_representation.getCenter()(0) << " " << simplified_grain_representation.getCenter()(1) << std::endl;

        simplified_grain_representations.push_back(simplified_grain_representation);
    }

    SimplifiedGrainManipulator<dim> simplified_grain_manipulator;

    if (currentIncrement_pf > 0 || userInputs_pf.load_grain_structure){
        simplified_grain_manipulator.transferGrainIds(old_grain_representations, simplified_grain_representations);
    }

    simplified_grain_manipulator.reassignGrains(simplified_grain_representations, userInputs_pf.buffer_between_grains, userInputs_pf.variables_for_remapping);

    for (unsigned int g=0; g<this->simplified_grain_representations.size(); g++){
        pcout << "Grain: " << simplified_grain_representations[g].getGrainId() << " " << simplified_grain_representations[g].getOrderParameterId() << " Center: " << simplified_grain_representations[g].getCenter()(0) << " " << simplified_grain_representations[g].getCenter()(1) << std::endl;

    }

    OrderParameterRemapper<dim> order_parameter_remapper;
    order_parameter_remapper.remap(simplified_grain_representations, solutionSet, *dofHandlersSet_nonconst.at(scalar_field_index), FESet.at(scalar_field_index)->dofs_per_cell, userInputs_pf.buffer_between_grains);

    pcout << "Reassigning grains completed." << std::endl << std::endl;

    //end log
    computing_timer_pf.leave_subsection("multiPhysicsBVP: reassignGrains");


}

#include "../../include/multiPhysicsBVP_template_instantiations.h"
