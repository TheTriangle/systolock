using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SystoLockClient
{
    internal class User
    {
        public String firstName { get; set; }
        public String lastName { get; set; }
        public String initials { get; set; }
        public String displayName { get; set; }
        public String description { get; set; }
        public String office { get; set; }
        public String telephoneNumber { get; set; }
        public String EMail { get; set; }
        public String webPage { get; set; }
        public String creationDate { get; set; }

        public User(string firstName, string lastName, string initials, string displayName, string description, string office, string telephoneNumber, string eMail, string webPage, String creationDate)
        {
            this.firstName = firstName;
            this.lastName = lastName;
            this.initials = initials;
            this.displayName = displayName;
            this.description = description;
            this.office = office;
            this.telephoneNumber = telephoneNumber;
            EMail = eMail;
            this.webPage = webPage;
            this.creationDate = creationDate;
        }

        public User()
        {

        }
    }
}
