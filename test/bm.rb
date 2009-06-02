require 'bitarray'
require 'benchmark'

Benchmark.bm(28) { |bm|
  bm.report("BitArray initialize") { 10000.times { BitArray.new(256) } }
  s = "0"*256
  bm.report("BitArray init from string") { 10000.times { BitArray.new(s) } }
  a = [0]*256
  bm.report("BitArray init from array")  { 10000.times { BitArray.new(a) } }

  ba = BitArray.new(256)
  
  bm.report("BitArray []")         { 10000.times { ba[rand(256)] } }
  bm.report("BitArray []=")        { 10000.times { ba[rand(256)] = [0,1][rand(2)] } }
  bm.report("BitArray each")       { 10000.times { ba.each {|b| b } } }
  bm.report("BitArray to_s")       { 10000.times { ba.to_s } }
  bm.report("BitArray to_a")       { 10000.times { ba.to_a } }
  
  ba = BitArray.new(256)
  bm.report("BitArray total_set (none)") { 10000.times { ba.total_set } }
  ba.set_all_bits
  bm.report("BitArray total_set (all)")  { 10000.times { ba.total_set } }

  bm.report("BitArray set_all_bits")    { 10000.times { ba.set_all_bits} }
  bm.report("BitArray clear_all_bits")  { 10000.times { ba.clear_all_bits } }
  bm.report("BitArray toggle_bit")      { 10000.times { ba.toggle_bit(1) } }
  bm.report("BitArray toggle_all_bits") { 10000.times { ba.toggle_all_bits } }
  bm.report("BitArray clone")           { 10000.times { ba.clone } }
  bm.report("BitArray slice (beg,len)") { 10000.times { ba[17, 230] } }
  bm.report("BitArray slice (range)")   { 10000.times { ba[17..247] } }
  ba.set_all_bits
  bm.report("BitArray + (256, all set)") { 10000.times { ba + ba } }
  ba2 = BitArray.new(240)
  ba2.set_all_bits
  bm.report("BitArray + (240, all set)") { 10000.times { ba2 + ba2 } }
}

