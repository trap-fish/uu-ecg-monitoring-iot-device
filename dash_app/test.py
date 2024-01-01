import boto3
import pandas as pd

# AWS DynamoDB Configuration
dynamodb = boto3.resource('dynamodb', region_name='region-region-1')
table_name = '<table_name>'
table = dynamodb.Table(table_name)

response = table.scan()
items = response['Items']

df = pd.DataFrame(items)
print(df.head())