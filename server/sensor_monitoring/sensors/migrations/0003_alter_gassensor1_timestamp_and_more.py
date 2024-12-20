# Generated by Django 5.1.1 on 2024-09-29 14:58

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('sensors', '0002_gassensor1_gassensor2_delete_gassensor_and_more'),
    ]

    operations = [
        migrations.AlterField(
            model_name='gassensor1',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
        migrations.AlterField(
            model_name='gassensor2',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
        migrations.AlterField(
            model_name='humiditypressuresensor',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
        migrations.AlterField(
            model_name='lightsensor',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
        migrations.AlterField(
            model_name='soilmoisturesensor',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
        migrations.AlterField(
            model_name='vibrationsensor',
            name='timestamp',
            field=models.DateTimeField(auto_now_add=True),
        ),
    ]
