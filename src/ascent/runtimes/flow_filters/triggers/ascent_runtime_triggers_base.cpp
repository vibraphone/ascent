//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2015-2018, Lawrence Livermore National Security, LLC.
// 
// Produced at the Lawrence Livermore National Laboratory
// 
// LLNL-CODE-716457
// 
// All rights reserved.
// 
// This file is part of Ascent. 
// 
// For details, see: http://ascent.readthedocs.io/.
// 
// Please also read ascent/LICENSE
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, 
//   this list of conditions and the disclaimer below.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of the LLNS/LLNL nor the names of its contributors may
//   be used to endorse or promote products derived from this software without
//   specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


//-----------------------------------------------------------------------------
///
/// file: ascent_runtime_triggers_base.cpp
///
//-----------------------------------------------------------------------------

#include "ascent_runtime_triggers_base.hpp"

//-----------------------------------------------------------------------------
// thirdparty includes
//-----------------------------------------------------------------------------

// conduit includes
#include <conduit.hpp>
#include <conduit_blueprint.hpp>

//-----------------------------------------------------------------------------
// ascent includes
//-----------------------------------------------------------------------------
#include <ascent_logging.hpp>
#include <flow_graph.hpp>
#include <flow_workspace.hpp>

// mpi
#ifdef ASCENT_MPI_ENABLED
#include <mpi.h>
#endif

using namespace conduit;
using namespace std;

using namespace flow;

//-----------------------------------------------------------------------------
// -- begin ascent:: --
//-----------------------------------------------------------------------------
namespace ascent
{

//-----------------------------------------------------------------------------
// -- begin ascent::runtime --
//-----------------------------------------------------------------------------
namespace runtime
{

//-----------------------------------------------------------------------------
// -- begin ascent::runtime::filters --
//-----------------------------------------------------------------------------
namespace filters
{

;
//-----------------------------------------------------------------------------
TriggerFilter::TriggerFilter()
:Filter()
{
// empty
}

//-----------------------------------------------------------------------------
TriggerFilter::~TriggerFilter()
{
// empty
}

bool   
TriggerFilter::verify_params(const conduit::Node &params,
                             conduit::Node &info)
{
    bool res = true;
    if(! params.has_child("actions"))
    {
        info["errors"].append() = "Missing required conduit::Node parameter 'actions'";
        res = false;
    }
    return res;
}
//-----------------------------------------------------------------------------
void 
TriggerFilter::declare_interface(Node &i)
{
    i["type_name"]   = this->get_type_name();
    i["port_names"].append() = "in";
    i["output_port"] = "false";
}

//-----------------------------------------------------------------------------
void 
TriggerFilter::execute()
{
  if(!input(0).check_type<conduit::Node>())
  {
      ASCENT_ERROR("TriggerFilter input must be a conduit blueprint data set");
  }

  const conduit::Node &data = this->get_data();
 
  // triggered better be all true of all false amongst all ranks
  bool triggered = this->trigger(data);

  if(triggered)
  {
    //
    // Run Ascent and execute the trigger actions
    //
    
    conduit::Node *in = input<conduit::Node>(0);
    conduit::Node actions = params()["actions"];
    Ascent ascent;

    Node ascent_opts;
    ascent_opts["runtime/type"] = "ascent";
    ascent.open(ascent_opts);
    ascent.publish(*in);
    ascent.execute(actions);
    ascent.close();
  }

}

//-----------------------------------------------------------------------------
FieldTriggerFilter::FieldTriggerFilter()
: TriggerFilter()
{

}

//-----------------------------------------------------------------------------
FieldTriggerFilter::~FieldTriggerFilter()
{

}
    
//-----------------------------------------------------------------------------
bool   
FieldTriggerFilter::verify_params(const conduit::Node &params,
                                  conduit::Node &info)
{
    bool res = TriggerFilter::verify_params(params, info);
    if(! params.has_child("field") || 
       ! params["field"].dtype().is_string() )
    {
        info["errors"].append() = "Missing required string parameter 'field'";
        res = false;
    }
    std::cout<<"Verified field\n";
    return res;
}

//-----------------------------------------------------------------------------
const conduit::Node &
FieldTriggerFilter::get_data()
{
  ASCENT_INFO("Field Trigger!");

  std::string field_name = params()["field"].as_string();
  conduit::Node *data = input<conduit::Node>(0);
  //data->print(); 
  std::string field_path = "fields/" + field_name; 
  // TODO: assume multi-dom
  if(!data->has_path(field_path))
  {
    (*data)[field_path].print(); 
    ASCENT_ERROR("Field trigger: data set does not contain field '"<<field_name<<"'"); 
  }
  
  const conduit::Node &field = (*data)["fields/"+field_name];
  
  return field;
}
    
//-----------------------------------------------------------------------------
PerformanceTriggerFilter::PerformanceTriggerFilter()
: FieldTriggerFilter()
{

}

//-----------------------------------------------------------------------------
PerformanceTriggerFilter::~PerformanceTriggerFilter()
{

}
    
//-----------------------------------------------------------------------------
void 
PerformanceTriggerFilter::execute()
{
  if(!input(0).check_type<conduit::Node>())
  {
      ASCENT_ERROR("TriggerFilter input must be a conduit blueprint data set");
  }

  const conduit::Node &data = this->get_data();
 
  // triggered better be all true of all false amongst all ranks
  bool triggered = this->trigger(data);

  if(triggered)
  {
    //
    // Run Ascent and execute the trigger actions
    //
    std::cout<<"Triggerd!!!\n";  
    conduit::Node *in = input<conduit::Node>(0);
    in->print();
    conduit::Node actions = params()["actions"];
    Ascent ascent;

    Node ascent_opts;
#ifdef ASCENT_MPI_ENABLED
    ascent_opts["mpi_comm"] = Workspace::default_mpi_comm();
#endif
    ascent_opts["runtime/type"] = "ascent";
    ascent.open(ascent_opts);
    ascent.publish(*in);
    ascent.execute(actions);
    ascent.close();
  }

}
//-----------------------------------------------------------------------------
//void 
//FieldTrigger::get_data()
//{
//  conduit::Node *dataset = input<Node>(0);
//  //dataset->print();
// 
//  std::vector<std::string> field_names = (*dataset)["fields"].child_names(); 
//  for(int f = 0; f < field_names.size(); ++f)
//  {
//    std::cout<<"Data set has field "<<field_names[f]<<"\n";
//  }
//
//  const Node &field = (*dataset)["fields/density"];
//  //field.print();
//  // TODO generalize to any data type
//  // this is not always a double
//  const float64 *vals = field["values"].as_float64_ptr();
//
//  const int32 field_size = field["values"].dtype().number_of_elements();
//  std::cout<<"number of elements "<<field_size<<"\n";
//  
//  // do stuff
//}

//-----------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------
// -- end ascent::runtime::filters --
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------
// -- end ascent::runtime --
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------
// -- end ascent:: --
//-----------------------------------------------------------------------------




