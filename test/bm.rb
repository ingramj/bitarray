require 'bitfield'
require 'bitarray'
require 'benchmark'

Benchmark.bm(28) { |bm|
  puts "---------------------------- Object instantiation (10,000 iterations)"
  bm.report("BitField initialize") { 10000.times { BitField.new(256) } }
  bm.report("BitArray initialize") { 10000.times { BitArray.new(256) } }

  bf = BitField.new(256)
  ba = BitArray.new(256)
  
  puts "---------------------------- Element Reading (10,000 iterations)"
  bm.report("BitField []")         { 10000.times { bf[rand(256)] } }
  bm.report("BitArray []")         { 10000.times { ba[rand(256)] } }

  puts "---------------------------- Element Writing (10,000 iterations)"
  bm.report("BitField []=")        { 10000.times { bf[rand(256)] = [0,1][rand(2)] } }
  bm.report("BitArray []=")        { 10000.times { ba[rand(256)] = [0,1][rand(2)] } }

  puts "---------------------------- Element Enumeration (10,000 iterations)"
  bm.report("BitField each")       { 10000.times { bf.each {|b| b } } }
  bm.report("BitArray each")       { 10000.times { ba.each {|b| b } } }

  puts "---------------------------- To String (10,000 iterations)"
  bm.report("BitField to_s")       { 10000.times { bf.to_s } }
  bm.report("BitArray to_s")       { 10000.times { ba.to_s } }

  puts "---------------------------- To Array (10,000 iterations)"
  bm.report("BitField to_a")       { 10000.times { bf.to_a } }
  bm.report("BitArray to_a")       { 10000.times { ba.to_a } }

  puts "---------------------------- Total Set (100,000 iterations)"
  bf = BitField.new(256)
  ba = BitArray.new(256)
  bm.report("BitField total_set (none)") { 100000.times { bf.total_set } }
  bm.report("BitArray total_set (none)") { 100000.times { ba.total_set } }
  bf.each {|b| b = 1}
  ba.set_all_bits
  bm.report("BitField total_set (all)") { 100000.times { bf.total_set } }
  bm.report("BitArray total_set (all)") { 100000.times { ba.total_set } }

  puts "---------------------------- BitArray methods (100,000 iterations)"
  bm.report("BitArray set_all_bits")    { 100000.times { ba.set_all_bits} }
  bm.report("BitArray clear_all_bits")  { 100000.times { ba.clear_all_bits } }
  bm.report("BitArray toggle_bit")      { 100000.times { ba.toggle_bit(1) } }
  bm.report("BitArray toggle_all_bits") { 100000.times { ba.toggle_all_bits } }
  bm.report("BitArray clone")           { 100000.times { ba.clone } }
  bm.report("BitArray slice (beg,len)") { 100000.times { ba[17, 230] } }
  bm.report("BitArray slice (range)")   { 100000.times { ba[17..247] } }
  ba.set_all_bits
  bm.report("BitArray + (256, all set)") { 100000.times { ba + ba } }
  ba2 = BitArray.new(240)
  ba2.set_all_bits
  bm.report("BitArray + (240, all set)") { 100000.times { ba2 + ba2 } }
}

