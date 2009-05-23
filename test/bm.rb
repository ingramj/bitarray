require 'bitfield'
require 'bitarray'
require 'benchmark'

Benchmark.bm(20) { |bm|
  puts "-------------------- Object instantiation."
  bm.report("BitField initialize") { 10000.times { BitField.new(256) } }
  bm.report("BitArray initialize") { 10000.times { BitArray.new(256) } }

  bf = BitField.new(256)
  ba = BitArray.new(256)
  
  puts "-------------------- Element Reading"
  bm.report("BitField []")         { 10000.times { bf[rand(256)] } }
  bm.report("BitArray []")         { 10000.times { ba[rand(256)] } }

  puts "-------------------- Element Writing"
  bm.report("BitField []=")        { 10000.times { bf[rand(256)] = [0,1][rand(2)] } }
  bm.report("BitArray []=")        { 10000.times { ba[rand(256)] = [0,1][rand(2)] } }

  puts "-------------------- Element Enumeration"
  bm.report("BitField each")       { 10000.times { bf.each {|b| b } } }
  bm.report("BitArray each")       { 10000.times { ba.each {|b| b } } }

  puts "------------------- To String"
  bm.report("BitField to_s")       { 10000.times { bf.to_s } }
  bm.report("BitArray to_s")       { 10000.times { ba.to_s } }
}

