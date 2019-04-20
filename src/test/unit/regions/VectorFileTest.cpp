/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2018, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 
 * Author: David Keeney, April, 2019
 * ---------------------------------------------------------------------
 */
 
/*---------------------------------------------------------------------
  * This is a test of the VectorFileEffector and VectorFileSensor modules.  
  *
  * For those not familiar with GTest:
  *     ASSERT_TRUE(value)   -- Fatal assertion that the value is true.  Test terminates if false.
  *     ASSERT_FALSE(value)   -- Fatal assertion that the value is false. Test terminates if true.
  *     ASSERT_STREQ(str1, str2)   -- Fatal assertion that the strings are equal. Test terminates if false.
  *
  *     EXPECT_TRUE(value)   -- Nonfatal assertion that the value is true.  Test fails but continues if false.
  *     EXPECT_FALSE(value)   -- Nonfatal assertion that the value is false. Test fails but continues if true.
  *     EXPECT_STREQ(str1, str2)   -- Nonfatal assertion that the strings are equal. Test fails but continues if false.
  *
  *     EXPECT_THROW(statement, exception_type) -- nonfatal exception, cought and continues.
  *---------------------------------------------------------------------
  */


#include <nupic/engine/NuPIC.hpp>
#include <nupic/engine/Network.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/engine/Input.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/RegisteredRegionImpl.hpp>
#include <nupic/engine/RegisteredRegionImplCpp.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/types/Exception.hpp>
#include <nupic/os/Env.hpp>
#include <nupic/os/Path.hpp>
#include <nupic/os/Timer.hpp>
#include <nupic/os/Directory.hpp>
#include <nupic/engine/YAMLUtils.hpp>
#include <nupic/regions/SPRegion.hpp>


#include <string>
#include <vector>
#include <cmath> // fabs/abs
#include <cstdlib> // exit
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <streambuf>
#include <iterator>
#include <algorithm>



#include "yaml-cpp/yaml.h"
#include "gtest/gtest.h"
#include "RegionTestUtilities.hpp"

#define VERBOSE if(verbose)std::cerr << "[          ] "
static bool verbose = true;  // turn this on to print extra stuff for debugging the test.

// The following string should contain a valid expected Spec - manually verified. 
#define EXPECTED_EFFECTOR_SPEC_COUNT  1  // The number of parameters expected in the VectorFileEffector Spec
#define EXPECTED_SENSOR_SPEC_COUNT  9    // The number of parameters expected in the VectorFileSensor Spec

using namespace nupic;
namespace testing 
{

  static bool compareFiles(const std::string& p1, const std::string& p2);  // forward declaration, find body below
	static void createTestData();                                            // forward declaration, find body below


  // Verify that all parameters are working.
  // Assumes that the default value in the Spec is the same as the default 
  // when creating a region with default constructor.
  TEST(VectorFileTest, testSpecAndParametersEffector)
  {
    Network net;

    // create an Effector region with default parameters
    std::shared_ptr<Region> region1 = net.addRegion("region1", "VectorFileEffector", "");  // use default configuration

		std::set<std::string> excluded;
    checkGetSetAgainstSpec(region1, EXPECTED_EFFECTOR_SPEC_COUNT, excluded, verbose);
    checkInputOutputsAgainstSpec(region1, verbose);

  }
  TEST(VectorFileTest, testSpecAndParametersSensor)
  {
    Network net;

    // create an Sensor region with default parameters
    std::shared_ptr<Region> region1 = net.addRegion("region1", "VectorFileSensor", "");  // use default configuration

		std::set<std::string> excluded;
    checkGetSetAgainstSpec(region1, EXPECTED_SENSOR_SPEC_COUNT, excluded, verbose);
    checkInputOutputsAgainstSpec(region1, verbose);

  }


	TEST(SPRegionTest, testLinking)
	{
    // This is a minimal end-to-end test containing an Effector and a Sensor region
    // this test will hook up the VectorFileSensor to a VectorFileEffector to capture the results.
    //
    std::string test_input_file = "TestOutputDir/TestInput.csv";
    std::string test_output_file = "TestOutputDir/TestOutput.csv";
		createTestData(test_input_file);


    VERBOSE << "Setup Network; add 2 regions and 1 link." << std::endl;
	  Network net;

    // Explicit parameters:  (Yaml format...but since YAML is a superset of JSON, 
    // you can use JSON format as well)

    std::shared_ptr<Region> region1 = net.addRegion("region1", "VectorFileSensor", "{activeOutputCount: "+std::to_string(dataWidth) +"}");
    std::shared_ptr<Region> region3 = net.addRegion("region3", "VectorFileEffector", "{outputFile: '"+ test_output_file + "'}");


    net.link("region1", "region3", "", "", "dataOut", "dataIn");

    VERBOSE << "Load Data." << std::endl;
    region1->executeCommand({ "loadFile", test_input_file });


    VERBOSE << "Initialize." << std::endl;
    net.initialize();

    VERBOSE << "Execute once." << std::endl;
    net.run(1);

	  VERBOSE << "Checking data after first iteration..." << std::endl;
    VERBOSE << "  VectorFileSensor Output" << std::endl;
    Array r1OutputArray = region1->getOutputData("dataOut");
    EXPECT_EQ(r1OutputArray.getCount(), dataWidth);
    EXPECT_TRUE(r1OutputArray.getType() == NTA_BasicType_Real32)
            << "actual type is " << BasicType::getName(r1OutputArray.getType());

    Real32 *buffer1 = (Real32*) r1OutputArray.getBuffer();
		VERBOSE << "  VectorFileEffector output: " << r1OutputArray << std::endl;
    VERBOSE << "  SPRegion input: " << std::endl;
		

	  // execute network several more times and check that it has output.
    VERBOSE << "Execute 9 times." << std::endl;
    net.run(9);

    VERBOSE << "  VectorFileEffector input" << std::endl;
    Array r3InputArray = region3->getInputData("dataIn");
    ASSERT_TRUE(r3InputArray.getType() == NTA_BasicType_Real32)
      << "actual type is " << BasicType::getName(r3InputArray.getType());
    ASSERT_TRUE(r3InputArray.getCount() == columnCount);
    const Real32 *buffer4 = (const Real32*)r3InputArray.getBuffer();
    for (size_t i = 0; i < r3InputArray.getCount(); i++)
    {
      //VERBOSE << "  [" << i << "]=    " << buffer4[i] << "" << std::endl;
      ASSERT_TRUE(buffer1[i] == buffer4[i])
        << " Buffer content different. Element[" << i << "] from Sensor out is " << buffer1[i] << ", input to VectorFileEffector is " << buffer4[i];
    }

    // cleanup
    region3->executeCommand({ "closeFile" });
		
		// Compare files
    ASSERT_TRUE(compareFiles(test_input_file, test_output_file)) << "Files should be the same.";
}


TEST(SPRegionTest, testSerialization)
{
    std::string test_input_file = "TestOutputDir/TestInput.csv";
    std::string test_output_file = "TestOutputDir/TestOutput.csv";
		createTestData(test_input_file);
		
	  // use default parameters the first time
	  Network net1;
	  Network net3;

	  VERBOSE << "Setup first network and save it" << std::endl;
    std::shared_ptr<Region> n1region1 = net.addRegion("region1", "VectorFileSensor", "{activeOutputCount: "+std::to_string(dataWidth) +"}");
    std::shared_ptr<Region> n1region3 = net.addRegion("region3", "VectorFileEffector", "{outputFile: '"+ test_output_file + "'}");
    net.link("region1", "region3", "", "", "dataOut", "dataIn");

    VERBOSE << "Load Data." << std::endl;
    n1region1->executeCommand({ "loadFile", test_input_file });
    net1.initialize();

		net1.run(1);

    Directory::removeTree("TestOutputDir", true);
	  net1.saveToFile_ar("TestOutputDir/VectorFileTest.stream");

	  VERBOSE << "Restore into a second network and compare." << std::endl;
    net2.loadFromFile("TestOutputDir/VectorFileTest.stream");
	  std::shared_ptr<Region> n3region1 = net3.getRegion("region1");
	  std::shared_ptr<Region> n3region3 = net3.getRegion("region3");

    EXPECT_TRUE(n1region1 == n3region1);
    EXPECT_TRUE(n1region3 == n3region3);

    // cleanup
    Directory::removeTree("TestOutputDir", true);

	}
	
	//////////////////////////////////////////////////////////////////////////////////

	static bool compareFiles(const std::string& p1, const std::string& p2) {
	  std::ifstream f1(p1, std::ifstream::binary|std::ifstream::ate);
	  std::ifstream f2(p2, std::ifstream::binary|std::ifstream::ate);

	  if (f1.fail() || f2.fail()) {
	    return false; //file problem
	  }

	  if (f1.tellg() != f2.tellg()) {
	    return false; //size mismatch
	  }

	  //seek back to beginning and use std::equal to compare contents
	  f1.seekg(0, std::ifstream::beg);
	  f2.seekg(0, std::ifstream::beg);
	  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
	                    std::istreambuf_iterator<char>(),
	                    std::istreambuf_iterator<char>(f2.rdbuf()));
	}
	
	static void createTestData(const std::string& test_input_file) {
	    // make a place to put test data.
    if (!Directory::exists("TestOutputDir")) Directory::create("TestOutputDir", false, true); 
    if (Path::exists(test_input_file)) Path::remove(test_input_file);
    if (Path::exists(test_output_file)) Path::remove(test_output_file);

    // Create a csv file to use as input.
    // The SDR data we will feed it will be a matrix with 1's on the diagonal
    // and we will feed it one row at a time, for 10 rows.
    size_t dataWidth = 10;
    size_t dataRows = 10;
    std::ofstream  f(test_input_file.c_str());
    for (size_t i = 0; i < 10; i++) {
      for (size_t j = 0; j < dataWidth; j++) {
        if ((j % dataRows) == i) f << "1.0,";
        else f << "0.0,";
      }
      f << std::endl;
    }
    f.close();
  }


} // namespace

