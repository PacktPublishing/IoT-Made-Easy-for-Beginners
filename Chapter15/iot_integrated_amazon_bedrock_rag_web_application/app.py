from flask import Flask, request, jsonify, render_template
import numpy as np
from langchain.embeddings import BedrockEmbeddings
from langchain.llms.bedrock import Bedrock
from langchain.vectorstores import FAISS
from langchain.indexes.vectorstore import VectorStoreIndexWrapper
from langchain.document_loaders import PyPDFDirectoryLoader
from langchain.text_splitter import RecursiveCharacterTextSplitter
import boto3
import ssl
import os
from urllib.request import urlretrieve

# Create the Flask application
app = Flask(__name__)

# Create the Boto3 client
print("Initializing Boto3 client...")
bedrock_runtime = boto3.client(
    service_name='bedrock-runtime',
    aws_access_key_id='<YOUR_AWS_ACCESS_KEY_ID_HERE>',
    aws_secret_access_key='<YOUR_AWS_SECRET_ACCESS_KEY_HERE>',
    region_name='us-west-2'
)
print("Boto3 client initialized.")

# Initialize embeddings and language model
llm = Bedrock(model_id="anthropic.claude-v2", client=bedrock_runtime, model_kwargs={'max_tokens_to_sample':200})
bedrock_embeddings = BedrockEmbeddings(model_id="amazon.titan-embed-text-v1", client=bedrock_runtime)

# Load and preprocess documents
print("Downloading files...")
ssl._create_default_https_context = ssl._create_unverified_context
os.makedirs("data", exist_ok=True)
files = [
    "https://www.cisa.gov/sites/default/files/publications/Federal_Government_Cybersecurity_Incident_and_Vulnerability_Response_Playbooks_508C.pdf",
    "https://www.cisa.gov/sites/default/files/2023-10/StopRansomware-Guide-508C-v3_1.pdf",
]
print("Files downloaded.")

for url in files:
    file_path = os.path.join("data", url.rpartition("/")[2])
    urlretrieve(url, file_path)

loader = PyPDFDirectoryLoader("./data/")
documents = loader.load()
text_splitter = RecursiveCharacterTextSplitter(chunk_size=1000, chunk_overlap=100)
text_to_process = text_splitter.split_documents(documents)
vectorstore_faiss = FAISS.from_documents(text_to_process, bedrock_embeddings)

# Define route for the home page with a form
@app.route('/', methods=['GET'])
def home():
    return render_template('query_form.html')

# Define route to handle form submissions
@app.route('/query', methods=['POST'])
def handle_query():
    try:
        query = request.form['query']
        query_embedding = vectorstore_faiss.embedding_function(query)
        matched_documents = vectorstore_faiss.similarity_search_by_vector(query_embedding)

        response = {
            'matched_documents': [{'content': doc.page_content} for doc in matched_documents]
        }
        return jsonify(response)
    except Exception as e:
        return jsonify(error=str(e)), 500

# Run the Flask application
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
