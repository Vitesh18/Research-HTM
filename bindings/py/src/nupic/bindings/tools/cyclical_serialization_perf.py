#!/usr/bin/env python
# ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2017, Numenta, Inc.  Unless you have an agreement
# with Numenta, Inc., for a separate license for this software code, the
# following terms and conditions apply:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
#
# http://numenta.org/licenses/
# ----------------------------------------------------------------------

"""Capnp serialization performance test that involves a network that contains
a simple PyRegion which in turn contains an extension-based Random instance.
"""

import json
import time

# NOTE need to import capnp first to activate the magic necessary for
# NetworkProto_capnp, etc.
import capnp
from nupic.proto.NetworkProto_capnp import NetworkProto

import nupic.bindings.engine_internal as engine
from nupic.bindings.tools.serialization_test_py_region import \
     SerializationTestPyRegion



_SERIALIZATION_LOOPS = 100000
_DESERIALIZATION_LOOPS = 100000



def _runTest():
  net = engine.Network()
  net.addRegion(SerializationTestPyRegion.__name__,
                "py." + SerializationTestPyRegion.__name__,
                json.dumps({
                  "dataWidth": 128,
                  "randomSeed": 99,
                }))

  # Measure serialization
  startSerializationTime = time.time()

  builderProto = NetworkProto.new_message()
  for i in xrange(_SERIALIZATION_LOOPS):
    net.write(builderProto)

  elapsedSerializationTime = time.time() - startSerializationTime


  readerProto = NetworkProto.from_bytes(builderProto.to_bytes())


  # Measure deserialization
  startDeserializationTime = time.time()

  for i in xrange(_DESERIALIZATION_LOOPS):
    net.read(readerProto)

  elapsedDeserializationTime = time.time() - startDeserializationTime

  # Print report
  print _SERIALIZATION_LOOPS, "Serialization loops in", \
        elapsedSerializationTime, "seconds."
  print "\t", elapsedSerializationTime/_SERIALIZATION_LOOPS, "seconds per loop."

  print _DESERIALIZATION_LOOPS, "Deserialization loops in", \
        elapsedDeserializationTime, "seconds."
  print "\t", elapsedDeserializationTime/_DESERIALIZATION_LOOPS, "seconds per loop."



def main():
  """Measure capnp serialization performance of a network containing a simple
  python region that in-turn contains a Random instance.
  """
  engine.Network.registerPyRegion(__name__,
                                    SerializationTestPyRegion.__name__)

  try:
    _runTest()
  finally:
    engine.Network.unregisterPyRegion(SerializationTestPyRegion.__name__)

if __name__ == "__main__":
  main()
