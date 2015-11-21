require 'spec_helper'

RSpec.describe Ripo::Image, '.detect_format' do
  context 'Given "fixtures/vimlogo-141x141.gif"' do
    it 'returns :gif' do
      expect(Ripo::Image.detect_format("spec/fixtures/vimlogo-141x141.gif")).to eq(:gif)
    end
  end

  context 'Given "fixtures/momosan.jpg"' do
    it 'returns :jpeg' do
      expect(Ripo::Image.detect_format("spec/fixtures/momosan.jpg")).to eq(:jpeg)
    end
  end

  context 'Given "fixtures/vimlogo-141x141.png"' do
    it 'returns :png' do
      expect(Ripo::Image.detect_format("spec/fixtures/vimlogo-141x141.png")).to eq(:png)
    end
  end

  context 'Given "fixtures/not_image.txt"' do
    it 'returns nil' do
      expect(Ripo::Image.detect_format("spec/fixtures/not_image.txt")).to eq(nil)
    end
  end
end
