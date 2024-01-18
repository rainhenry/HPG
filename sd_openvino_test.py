#!/usr/bin/python3
from optimum.intel.openvino import OVStableDiffusionPipeline
model_id="./Taiyi-Stable-Diffusion-1B-Chinese-EN-v0.1"
pipe = OVStableDiffusionPipeline.from_pretrained(model_id, export=True, compile=False)
pipe.to("cpu")
pipe.compile()
prompt = '猫'
image = pipe(prompt, guidance_scale=10).images[0]  
image.show()
image.save("test2.png")

