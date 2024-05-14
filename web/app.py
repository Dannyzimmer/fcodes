import tempfile
import gradio as gr

from PIL import Image

from fcodes.libs.classes.FamilyTree import FamilyTree

def process_file(uploaded_file):
    with tempfile.NamedTemporaryFile(delete=False) as output_file:
        tree = FamilyTree(uploaded_file)
        tree.render_tree(output_file.name, format = 'png')

        return Image.open(output_file.name + '.png')

def btn_click(input_file):
    if input_file is not None:
        return process_file(input_file)

with gr.Blocks() as demo:
    with gr.Row():
        inputs = gr.File(label="Upload TXT file")
    with gr.Row():
        with gr.Column(scale=1, min_width=300):
            btn = gr.Button("Get image!")
            output_image = gr.Image()

            btn.click(btn_click, inputs = inputs, outputs = output_image)

demo.launch()
