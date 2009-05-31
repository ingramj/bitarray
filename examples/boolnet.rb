#!/usr/bin/env ruby

require 'bitarray'

# A Random Boolean Network. Each bit of the network has two "neighbor" bits and
# an operation, all chosen at random. At each step, every bit is set to a new
# value by applying its operation to its neighbors.
#
# All such networks eventually fall into a cyclic or fixed-point attractor.
#
# See http://en.wikipedia.org/wiki/Boolean_network for more information.
class BoolNet
  attr_reader :state, :size
  def initialize(size = 80)
    @size = size
    # We use two arrays; one for the current state, and one holding infomation
    # used to update the state.
    @state = random_network(size)
    @update = random_update(size)
  end

  def step
    old_state = @state.clone
    @update.each_with_index { |u,i|
      case u[0]
      when :and
        @state[i] = old_state[u[1]] & old_state[u[2]]
      when :or
        @state[i] = old_state[u[1]] | old_state[u[2]]
      when :xor
        @state[i] = old_state[u[1]] ^ old_state[u[2]]
      end
    }
    return @state
  end
  
  def run(steps = 23)
    puts state
    steps.times {
      puts step
    }
  end

  private
  def random_network(size)
    ba = BitArray.new(size)
    0.upto(size - 1) {|b|
      if (rand(2) == 1)
        ba.set_bit b
      end
    }
    return ba
  end

  def random_update(size)
    # The update array is an array of [op, n1, n2] elements. op is the symbol
    # specifying which operation to use. n1 and n2 are indices of our neighbors
    # in the state array.
    update = Array.new(size)
    0.upto(size - 1) {|u|
      update[u] = [[:and, :or, :xor][rand(3)], rand(size), rand(size)]
    }
    return update
  end
end

if __FILE__ == $0
  BoolNet.new.run
end

