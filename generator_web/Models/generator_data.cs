namespace generator_web.Models
{
    public class generator_data
    {
        public int Id { get; set; }

        // Tüm float alanları float olarak tanımlayın
        public string CalismaDurumu { get; set; }
        public int OperationMode { get; set; }
        public long SistemCalismaSuresi { get; set; }

        // Şebeke verileri - FLOAT olarak
        public float SebekeVoltaj_l1 { get; set; }
        public float SebekeVoltaj_l2 { get; set; }
        public float SebekeVoltaj_l3 { get; set; }
        public float SebekeHz { get; set; }
        public float ToplamGuc { get; set; }
        public bool SebekeDurumu { get; set; }

        // Jeneratör verileri - FLOAT olarak
        public float GenVoltaj_l1 { get; set; }
        public float GenVoltaj_l2 { get; set; }
        public float GenVoltaj_l3 { get; set; }
        public float GenHz { get; set; }
        public float GenUretilenGuc { get; set; }
        public float GenGucFaktoru { get; set; }

        // Motor verileri - FLOAT olarak
        public float MotorRpm { get; set; }
        public float MotorSicaklik { get; set; }
        public float YagBasinci { get; set; }
        public float YakitSeviyesi { get; set; }
        public float BataryaVoltaji { get; set; }

        // Sistem durumu
        public long timestamp { get; set; }
        public bool KapatmaAlarmi { get; set; }
        public bool YukAtmaAlarmi { get; set; }
        public bool UyariAlarmi { get; set; }
        public bool SistemSaglikli { get; set; }


    }
}
