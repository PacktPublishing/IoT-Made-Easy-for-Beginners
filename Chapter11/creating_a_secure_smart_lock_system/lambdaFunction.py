# Import json and boto3 library and initialized boto3.client.
import json
import boto3

iot = boto3.client('iot-data')  # or your preferred region

def lambda_handler(event, context):
    dynamodb = boto3.resource('dynamodb')

    # Check if dynamodb "Authenticated_UID" table exist, if not create a new one
    table_name = 'Authenticated_UID'
    existing_tables = dynamodb.meta.client.list_tables()['TableNames']

    if table_name not in existing_tables:
        table = dynamodb.create_table(
            TableName=table_name,
            KeySchema=[
                {
                    'AttributeName': 'uid',
                    'KeyType': 'HASH' # Partition key
                },
                {
                    'AttributeName': 'device_id',
                    'KeyType': 'RANGE' # Sort key
                }
            ],
            AttributeDefinitions=[
                {
                    'AttributeName': 'uid',
                    'AttributeType': 'S'
                },
                {
                    'AttributeName': 'device_id',
                    'AttributeType': 'S'
                }
            ],
            ProvisionedThroughput={
                'ReadCapacityUnits': 5,
                'WriteCapacityUnits': 5
            }
        )

    # Use table "Authorized_UID" during the next processes
    table = dynamodb.Table('Authenticated_UID')

    # Get the encrypted UID data
    encryptedUid = str(event['encryptUid'])

    # Decrypt the encryptedUid data using shift = 3 value, same value with the sender client
    shift = 3
    decrypted = caesar_cipher_decrypt(encryptedUid, shift)

    # If switch state="Register", store the message data to dynamoDB
    if(str(event['state']) == "register"):
        # Store message to dynamoDB
        #table = dynamodb.Table('Authenticated_UID')

        # Parse MQTT message from AWS IoT
        dynamodb_message = {
            "uid": decrypted,
            "device_id": str(event['device_id'])
        }

        table.put_item(Item=dynamodb_message)

        return {
            'statusCode': 200,
            'body': json.dumps(f'Table {table_name} insert data success.')
        }

    else:
        # if authenticated, send back the device_id and UID back to the sender client
        # using MQTT publish. The UID will be encrypted first with
        # different value (shift = 4) than the received one.
        if authenticate(decrypted):
        #if is_authenticated:
            shift = 4
            encrypted = caesar_cipher_encrypt(decrypted, shift)

            # Prepare the return JSON message containing the device_id
            # and encrypted UID
            mqtt_message = {
                "device_id": str(event['device_id']),
                "encryptUid": encrypted
            }


            # Publish message to subscribe topic
            response = iot.publish(
                topic='rfid/1/sub',
                qos=0,
                payload=json.dumps(mqtt_message)
            )

            return {
                'statusCode': 200,
                'body': json.dumps('Message sent to rfid/1/sub')
            }
        return

# caesar_cipher encrypt and decrypt functions
def caesar_cipher_encrypt(message, shift):
    result = ""
    for i in range(len(message)):
        c = chr((ord(message[i]) - 32 + shift) % 95 + 32)
        result += c
    return result

def caesar_cipher_decrypt(message, shift):
    result = ""
    for i in range(len(message)):
        c = chr((ord(message[i]) - shift - 32) % 95 + 32)
        if c < ' ':
            c = chr(ord(c) + 95)
        result += c
    return result

# Create function to check if the UID exist in the authenticated list
def authenticate(uid):
    dynamodb = boto3.resource('dynamodb')
    table = dynamodb.Table('Authenticated_UID')
    response = table.get_item(
        Key={
        'uid': uid,
        'device_id': "1"
        }
    )
    return 'Item' in response
