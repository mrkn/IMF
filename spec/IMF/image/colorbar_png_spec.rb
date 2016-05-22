require 'spec_helper'

RSpec.describe IMF::Image do
  describe 'of "colorbar.png"' do
    subject(:image) do
      IMF::Image.open(image_filename)
    end

    let(:image_filename) do
      fixture_file('colorbar.png')
    end

    specify 'the size is 112x40' do
      expect(image.width).to eq(112)
      expect(image.height).to eq(40)
    end

    specify 'the color space is RGB' do
      expect(image.color_space).to eq(:RGB)
    end

    specify 'pixel channels is 4' do
      expect(image.pixel_channels).to eq(3)
    end

    specify 'component size is 1' do
      expect(image.component_size).to eq(1)
    end

    it 'does not have alpha channel' do
      expect(image).not_to have_alpha
    end

    it 'is correctly loaded' do
      expect(image[ 0,   0]).to eq([255, 255, 255])
      expect(image[ 0,  15]).to eq([255, 255, 255])
      expect(image[31,   0]).to eq([255, 255, 255])
      expect(image[31,  15]).to eq([255, 255, 255])

      expect(image[ 0,  16]).to eq([255, 255,   0])
      expect(image[ 0,  31]).to eq([255, 255,   0])
      expect(image[31,  16]).to eq([255, 255,   0])
      expect(image[31,  31]).to eq([255, 255,   0])

      expect(image[ 0,  32]).to eq([  0, 255, 255])
      expect(image[ 0,  47]).to eq([  0, 255, 255])
      expect(image[31,  32]).to eq([  0, 255, 255])
      expect(image[31,  47]).to eq([  0, 255, 255])

      expect(image[ 0,  48]).to eq([  0, 255,   0])
      expect(image[ 0,  63]).to eq([  0, 255,   0])
      expect(image[31,  48]).to eq([  0, 255,   0])
      expect(image[31,  63]).to eq([  0, 255,   0])

      expect(image[ 0,  64]).to eq([255,   0, 255])
      expect(image[ 0,  79]).to eq([255,   0, 255])
      expect(image[31,  64]).to eq([255,   0, 255])
      expect(image[31,  79]).to eq([255,   0, 255])

      expect(image[ 0,  80]).to eq([255,   0,   0])
      expect(image[ 0,  95]).to eq([255,   0,   0])
      expect(image[31,  80]).to eq([255,   0,   0])
      expect(image[31,  95]).to eq([255,   0,   0])

      expect(image[ 0,  96]).to eq([  0,   0, 255])
      expect(image[ 0, 111]).to eq([  0,   0, 255])
      expect(image[31,  96]).to eq([  0,   0, 255])
      expect(image[31, 111]).to eq([  0,   0, 255])

      expect(image[32,   0]).to eq([  0,   0, 255])
      expect(image[32,  15]).to eq([  0,   0, 255])
      expect(image[39,   0]).to eq([  0,   0, 255])
      expect(image[39,  15]).to eq([  0,   0, 255])

      expect(image[32,  16]).to eq([  0,   0,   0])
      expect(image[32,  31]).to eq([  0,   0,   0])
      expect(image[39,  16]).to eq([  0,   0,   0])
      expect(image[39,  31]).to eq([  0,   0,   0])

      expect(image[32,  32]).to eq([255,   0, 255])
      expect(image[32,  47]).to eq([255,   0, 255])
      expect(image[39,  32]).to eq([255,   0, 255])
      expect(image[39,  47]).to eq([255,   0, 255])

      expect(image[32,  48]).to eq([  0,   0,   0])
      expect(image[32,  63]).to eq([  0,   0,   0])
      expect(image[39,  48]).to eq([  0,   0,   0])
      expect(image[39,  63]).to eq([  0,   0,   0])

      expect(image[32,  64]).to eq([  0, 255, 255])
      expect(image[32,  79]).to eq([  0, 255, 255])
      expect(image[39,  64]).to eq([  0, 255, 255])
      expect(image[39,  79]).to eq([  0, 255, 255])

      expect(image[32,  80]).to eq([  0,   0,   0])
      expect(image[32,  95]).to eq([  0,   0,   0])
      expect(image[39,  80]).to eq([  0,   0,   0])
      expect(image[39,  95]).to eq([  0,   0,   0])

      expect(image[32,  96]).to eq([255, 255, 255])
      expect(image[32, 111]).to eq([255, 255, 255])
      expect(image[39,  96]).to eq([255, 255, 255])
      expect(image[39, 111]).to eq([255, 255, 255])
    end
  end

  describe 'of "colorbar_with_alpha.png"' do
    subject(:image) do
      IMF::Image.open(image_filename)
    end

    let(:image_filename) do
      fixture_file('colorbar_with_alpha.png')
    end

    specify 'the size is 112x40' do
      expect(image.width).to eq(112)
      expect(image.height).to eq(40)
    end

    specify 'the color space is RGB' do
      expect(image.color_space).to eq(:RGB)
    end

    specify 'pixel channels is 4' do
      expect(image.pixel_channels).to eq(4)
    end

    specify 'component size is 1' do
      expect(image.component_size).to eq(1)
    end

    it 'has alpha channel' do
      expect(image).to have_alpha
    end

    it 'is correctly loaded' do
      expect(image[ 0,   0]).to eq([255, 255, 255, 255])
      expect(image[ 0,  15]).to eq([255, 255, 255, 255])
      expect(image[31,   0]).to eq([255, 255, 255, 255])
      expect(image[31,  15]).to eq([255, 255, 255, 255])

      expect(image[ 0,  16]).to eq([255, 255,   0, 255])
      expect(image[ 0,  31]).to eq([255, 255,   0, 255])
      expect(image[31,  16]).to eq([255, 255,   0, 255])
      expect(image[31,  31]).to eq([255, 255,   0, 255])

      expect(image[ 0,  32]).to eq([  0, 255, 255, 255])
      expect(image[ 0,  47]).to eq([  0, 255, 255, 255])
      expect(image[31,  32]).to eq([  0, 255, 255, 255])
      expect(image[31,  47]).to eq([  0, 255, 255, 255])

      expect(image[ 0,  48]).to eq([  0, 255,   0, 255])
      expect(image[ 0,  63]).to eq([  0, 255,   0, 255])
      expect(image[31,  48]).to eq([  0, 255,   0, 255])
      expect(image[31,  63]).to eq([  0, 255,   0, 255])

      expect(image[ 0,  64]).to eq([255,   0, 255, 255])
      expect(image[ 0,  79]).to eq([255,   0, 255, 255])
      expect(image[31,  64]).to eq([255,   0, 255, 255])
      expect(image[31,  79]).to eq([255,   0, 255, 255])

      expect(image[ 0,  80]).to eq([255,   0,   0, 255])
      expect(image[ 0,  95]).to eq([255,   0,   0, 255])
      expect(image[31,  80]).to eq([255,   0,   0, 255])
      expect(image[31,  95]).to eq([255,   0,   0, 255])

      expect(image[ 0,  96]).to eq([  0,   0, 255, 255])
      expect(image[ 0, 111]).to eq([  0,   0, 255, 255])
      expect(image[31,  96]).to eq([  0,   0, 255, 255])
      expect(image[31, 111]).to eq([  0,   0, 255, 255])

      expect(image[32,   0]).to eq([  0,   0, 255, 255])
      expect(image[32,  15]).to eq([  0,   0, 255, 255])
      expect(image[39,   0]).to eq([  0,   0, 255, 255])
      expect(image[39,  15]).to eq([  0,   0, 255, 255])

      expect(image[32,  16]).to eq([  0,   0,   0, 255])
      expect(image[32,  31]).to eq([  0,   0,   0, 255])
      expect(image[39,  16]).to eq([  0,   0,   0, 255])
      expect(image[39,  31]).to eq([  0,   0,   0, 255])

      expect(image[32,  32]).to eq([255,   0, 255, 255])
      expect(image[32,  47]).to eq([255,   0, 255, 255])
      expect(image[39,  32]).to eq([255,   0, 255, 255])
      expect(image[39,  47]).to eq([255,   0, 255, 255])

      expect(image[32,  48]).to eq([  0,   0,   0, 255])
      expect(image[32,  63]).to eq([  0,   0,   0, 255])
      expect(image[39,  48]).to eq([  0,   0,   0, 255])
      expect(image[39,  63]).to eq([  0,   0,   0, 255])

      expect(image[32,  64]).to eq([  0, 255, 255, 255])
      expect(image[32,  79]).to eq([  0, 255, 255, 255])
      expect(image[39,  64]).to eq([  0, 255, 255, 255])
      expect(image[39,  79]).to eq([  0, 255, 255, 255])

      expect(image[32,  80]).to eq([  0,   0,   0, 255])
      expect(image[32,  95]).to eq([  0,   0,   0, 255])
      expect(image[39,  80]).to eq([  0,   0,   0, 255])
      expect(image[39,  95]).to eq([  0,   0,   0, 255])

      expect(image[32,  96]).to eq([255, 255, 255, 255])
      expect(image[32, 111]).to eq([255, 255, 255, 255])
      expect(image[39,  96]).to eq([255, 255, 255, 255])
      expect(image[39, 111]).to eq([255, 255, 255, 255])
    end
  end

  describe 'of "colorbar_with_colormap.png"' do
    subject(:image) do
      IMF::Image.open(image_filename)
    end

    let(:image_filename) do
      fixture_file('colorbar_with_colormap.png')
    end

    specify 'the size is 112x40' do
      expect(image.width).to eq(112)
      expect(image.height).to eq(40)
    end

    specify 'the color space is RGB' do
      expect(image.color_space).to eq(:RGB)
    end

    specify 'pixel channels is 4' do
      expect(image.pixel_channels).to eq(3)
    end

    specify 'component size is 1' do
      expect(image.component_size).to eq(1)
    end

    it 'does not have alpha channel' do
      expect(image).not_to have_alpha
    end

    it 'is correctly loaded' do
      expect(image[ 0,   0]).to eq([255, 255, 255])
      expect(image[ 0,  15]).to eq([255, 255, 255])
      expect(image[31,   0]).to eq([255, 255, 255])
      expect(image[31,  15]).to eq([255, 255, 255])

      expect(image[ 0,  16]).to eq([255, 255,   0])
      expect(image[ 0,  31]).to eq([255, 255,   0])
      expect(image[31,  16]).to eq([255, 255,   0])
      expect(image[31,  31]).to eq([255, 255,   0])

      expect(image[ 0,  32]).to eq([  0, 255, 255])
      expect(image[ 0,  47]).to eq([  0, 255, 255])
      expect(image[31,  32]).to eq([  0, 255, 255])
      expect(image[31,  47]).to eq([  0, 255, 255])

      expect(image[ 0,  48]).to eq([  0, 255,   0])
      expect(image[ 0,  63]).to eq([  0, 255,   0])
      expect(image[31,  48]).to eq([  0, 255,   0])
      expect(image[31,  63]).to eq([  0, 255,   0])

      expect(image[ 0,  64]).to eq([255,   0, 255])
      expect(image[ 0,  79]).to eq([255,   0, 255])
      expect(image[31,  64]).to eq([255,   0, 255])
      expect(image[31,  79]).to eq([255,   0, 255])

      expect(image[ 0,  80]).to eq([255,   0,   0])
      expect(image[ 0,  95]).to eq([255,   0,   0])
      expect(image[31,  80]).to eq([255,   0,   0])
      expect(image[31,  95]).to eq([255,   0,   0])

      expect(image[ 0,  96]).to eq([  0,   0, 255])
      expect(image[ 0, 111]).to eq([  0,   0, 255])
      expect(image[31,  96]).to eq([  0,   0, 255])
      expect(image[31, 111]).to eq([  0,   0, 255])

      expect(image[32,   0]).to eq([  0,   0, 255])
      expect(image[32,  15]).to eq([  0,   0, 255])
      expect(image[39,   0]).to eq([  0,   0, 255])
      expect(image[39,  15]).to eq([  0,   0, 255])

      expect(image[32,  16]).to eq([  0,   0,   0])
      expect(image[32,  31]).to eq([  0,   0,   0])
      expect(image[39,  16]).to eq([  0,   0,   0])
      expect(image[39,  31]).to eq([  0,   0,   0])

      expect(image[32,  32]).to eq([255,   0, 255])
      expect(image[32,  47]).to eq([255,   0, 255])
      expect(image[39,  32]).to eq([255,   0, 255])
      expect(image[39,  47]).to eq([255,   0, 255])

      expect(image[32,  48]).to eq([  0,   0,   0])
      expect(image[32,  63]).to eq([  0,   0,   0])
      expect(image[39,  48]).to eq([  0,   0,   0])
      expect(image[39,  63]).to eq([  0,   0,   0])

      expect(image[32,  64]).to eq([  0, 255, 255])
      expect(image[32,  79]).to eq([  0, 255, 255])
      expect(image[39,  64]).to eq([  0, 255, 255])
      expect(image[39,  79]).to eq([  0, 255, 255])

      expect(image[32,  80]).to eq([  0,   0,   0])
      expect(image[32,  95]).to eq([  0,   0,   0])
      expect(image[39,  80]).to eq([  0,   0,   0])
      expect(image[39,  95]).to eq([  0,   0,   0])

      expect(image[32,  96]).to eq([255, 255, 255])
      expect(image[32, 111]).to eq([255, 255, 255])
      expect(image[39,  96]).to eq([255, 255, 255])
      expect(image[39, 111]).to eq([255, 255, 255])
    end
  end
end
