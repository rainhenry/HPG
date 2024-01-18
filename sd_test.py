#!/usr/bin/python3
from diffusers import StableDiffusionPipeline
model_id="./Taiyi-Stable-Diffusion-1B-Chinese-EN-v0.1"
pipe = StableDiffusionPipeline.from_pretrained(model_id).to("cpu")
prompt = '猫'
image = pipe(prompt, guidance_scale=10).images[0]  
image.show()
image.save("test1.jpg")

